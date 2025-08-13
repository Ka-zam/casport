#ifndef INCLUDED_MONTE_CARLO_H
#define INCLUDED_MONTE_CARLO_H

#include "two_port.h"
#include "components.h"
#include "component_sweep.h"
#include <vector>
#include <random>
#include <functional>
#include <algorithm>

namespace cascadix {

// Distribution types for component tolerances
enum class distribution_type {
    UNIFORM,      // Uniform distribution
    GAUSSIAN,     // Normal distribution
    TRIANGULAR,   // Triangular distribution
    DISCRETE      // Discrete values only
};

// Component with tolerance specification
struct component_tolerance {
    component_type type;
    double nominal_value;
    double tolerance;  // As fraction (0.1 = ±10%)
    distribution_type distribution;
    double temperature_coefficient;  // ppm/°C
    
    component_tolerance(component_type t, double nom, double tol,
                       distribution_type dist = distribution_type::GAUSSIAN,
                       double temp_coeff = 0.0)
        : type(t), nominal_value(nom), tolerance(tol),
          distribution(dist), temperature_coefficient(temp_coeff) {}
    
    // Generate random value based on distribution
    double generate_value(std::mt19937& gen) const {
        double min_val = nominal_value * (1.0 - tolerance);
        double max_val = nominal_value * (1.0 + tolerance);
        
        switch (distribution) {
            case distribution_type::UNIFORM: {
                std::uniform_real_distribution<> dist(min_val, max_val);
                return dist(gen);
            }
            case distribution_type::GAUSSIAN: {
                // 3-sigma = tolerance range
                double sigma = (nominal_value * tolerance) / 3.0;
                std::normal_distribution<> dist(nominal_value, sigma);
                double val = dist(gen);
                // Clamp to tolerance range
                return std::max(min_val, std::min(max_val, val));
            }
            case distribution_type::TRIANGULAR: {
                // Simple triangular: peak at nominal
                std::uniform_real_distribution<> dist(0.0, 1.0);
                double u = dist(gen);
                if (u < 0.5) {
                    return min_val + std::sqrt(u * 2.0) * (nominal_value - min_val);
                } else {
                    return max_val - std::sqrt((1.0 - u) * 2.0) * (max_val - nominal_value);
                }
            }
            case distribution_type::DISCRETE: {
                // E96 series values (1% tolerance)
                // Simplified: just return closest standard value
                return nominal_value;  // TODO: Implement E-series rounding
            }
            default:
                return nominal_value;
        }
    }
};

// Monte Carlo simulation results
struct monte_carlo_results {
    size_t num_samples;
    std::vector<std::vector<double>> component_values;  // [sample][component]
    std::vector<complex> impedances;  // Input impedances
    std::vector<s_parameters> s_params;  // S-parameters
    std::vector<double> probabilities;  // Statistical weights
    
    // Statistical analysis
    complex mean_impedance;
    complex std_impedance;
    double yield_rate;  // Percentage meeting specs
    
    // Get percentile impedance
    complex get_percentile_impedance(double percentile) const {
        if (impedances.empty()) return complex(0, 0);
        
        size_t index = static_cast<size_t>(percentile * impedances.size() / 100.0);
        index = std::min(index, impedances.size() - 1);
        
        // Sort by magnitude for percentile
        std::vector<std::pair<double, complex>> sorted;
        for (const auto& z : impedances) {
            sorted.push_back({std::abs(z), z});
        }
        std::sort(sorted.begin(), sorted.end(),
                 [](const auto& a, const auto& b) { return a.first < b.first; });
        
        return sorted[index].second;
    }
    
    // Get VSWR distribution
    std::vector<double> get_vswr_distribution(double z0) const {
        std::vector<double> vswr_values;
        vswr_values.reserve(s_params.size());
        
        for (const auto& s : s_params) {
            vswr_values.push_back(s.vswr());
        }
        
        return vswr_values;
    }
    
    // GPU-friendly flattened arrays
    std::vector<float> get_flattened_impedances() const {
        std::vector<float> flat;
        flat.reserve(impedances.size() * 2);
        
        for (const auto& z : impedances) {
            flat.push_back(static_cast<float>(z.real()));
            flat.push_back(static_cast<float>(z.imag()));
        }
        
        return flat;
    }
    
    std::vector<float> get_smith_coordinates(double z0) const {
        std::vector<float> coords;
        coords.reserve(impedances.size() * 2);
        
        for (const auto& z : impedances) {
            complex z_norm = z / z0;
            complex gamma = (z_norm - 1.0) / (z_norm + 1.0);
            coords.push_back(static_cast<float>(gamma.real()));
            coords.push_back(static_cast<float>(gamma.imag()));
        }
        
        return coords;
    }
};

// Monte Carlo analyzer
class monte_carlo_analyzer {
public:
    monte_carlo_analyzer(size_t num_samples = 1000, unsigned seed = 0)
        : m_num_samples(num_samples), m_gen(seed ? seed : std::random_device{}()) {}
    
    // Add component with tolerance
    void add_component(const component_tolerance& comp) {
        m_components.push_back(comp);
    }
    
    // Run Monte Carlo simulation
    monte_carlo_results analyze(double frequency, double z0_system = 50.0,
                               const complex& z_load = complex(50.0, 0.0)) {
        monte_carlo_results results;
        results.num_samples = m_num_samples;
        results.component_values.reserve(m_num_samples);
        results.impedances.reserve(m_num_samples);
        results.s_params.reserve(m_num_samples);
        results.probabilities.reserve(m_num_samples);
        
        // Generate samples
        for (size_t i = 0; i < m_num_samples; ++i) {
            // Generate component values
            std::vector<double> values;
            values.reserve(m_components.size());
            
            for (const auto& comp : m_components) {
                values.push_back(comp.generate_value(m_gen));
            }
            results.component_values.push_back(values);
            
            // Build network
            two_port network = identity_two_port();
            for (size_t j = 0; j < m_components.size(); ++j) {
                two_port comp_network = create_component_network(
                    m_components[j].type, values[j], frequency);
                network = network * comp_network;
            }
            
            // Calculate parameters
            complex z_in = network.input_impedance(z_load);
            results.impedances.push_back(z_in);
            
            auto s = network.to_s_parameters(z0_system);
            results.s_params.push_back(s);
            
            // Equal probability for now (could add importance sampling)
            results.probabilities.push_back(1.0 / m_num_samples);
        }
        
        // Calculate statistics
        calculate_statistics(results);
        
        return results;
    }
    
    // Batch analysis for GPU processing
    std::vector<float> generate_batch_samples() {
        std::vector<float> samples;
        samples.reserve(m_num_samples * m_components.size());
        
        for (size_t i = 0; i < m_num_samples; ++i) {
            for (const auto& comp : m_components) {
                samples.push_back(static_cast<float>(comp.generate_value(m_gen)));
            }
        }
        
        return samples;
    }
    
    // Temperature analysis
    monte_carlo_results analyze_temperature(double frequency, double temp_min, double temp_max,
                                           size_t temp_steps, double z0_system = 50.0) {
        monte_carlo_results combined_results;
        
        double temp_step = (temp_max - temp_min) / (temp_steps - 1);
        
        for (size_t t = 0; t < temp_steps; ++t) {
            double temp = temp_min + t * temp_step;
            
            // Adjust component values for temperature
            auto temp_components = m_components;
            for (auto& comp : temp_components) {
                double temp_factor = 1.0 + comp.temperature_coefficient * (temp - 25.0) / 1e6;
                comp.nominal_value *= temp_factor;
            }
            
            // Run analysis at this temperature
            monte_carlo_analyzer temp_analyzer(m_num_samples / temp_steps, m_gen());
            for (const auto& comp : temp_components) {
                temp_analyzer.add_component(comp);
            }
            
            auto temp_results = temp_analyzer.analyze(frequency, z0_system);
            
            // Combine results
            combined_results.impedances.insert(combined_results.impedances.end(),
                                              temp_results.impedances.begin(),
                                              temp_results.impedances.end());
        }
        
        combined_results.num_samples = combined_results.impedances.size();
        calculate_statistics(combined_results);
        
        return combined_results;
    }
    
private:
    size_t m_num_samples;
    std::mt19937 m_gen;
    std::vector<component_tolerance> m_components;
    
    // Create component network from type and value
    two_port create_component_network(component_type type, double value, double freq) {
        switch (type) {
            case component_type::SERIES_R:
                return series_resistor(value);
            case component_type::SERIES_L:
                return series_inductor(value, freq);
            case component_type::SERIES_C:
                return series_capacitor(value, freq);
            case component_type::SHUNT_R:
                return shunt_resistor(value);
            case component_type::SHUNT_L:
                return shunt_inductor(value, freq);
            case component_type::SHUNT_C:
                return shunt_capacitor(value, freq);
            case component_type::TRANSMISSION_LINE:
                return transmission_line(value, 50.0, freq);  // Default Z0
            default:
                return identity_two_port();
        }
    }
    
    // Calculate statistical parameters
    void calculate_statistics(monte_carlo_results& results) {
        if (results.impedances.empty()) return;
        
        // Calculate mean
        complex sum(0, 0);
        for (const auto& z : results.impedances) {
            sum += z;
        }
        results.mean_impedance = sum / static_cast<double>(results.num_samples);
        
        // Calculate standard deviation
        complex sum_sq(0, 0);
        for (const auto& z : results.impedances) {
            complex diff = z - results.mean_impedance;
            sum_sq += complex(diff.real() * diff.real(), diff.imag() * diff.imag());
        }
        complex variance = sum_sq / static_cast<double>(results.num_samples - 1);
        results.std_impedance = complex(std::sqrt(variance.real()), std::sqrt(variance.imag()));
        
        // Calculate yield (example: VSWR < 2.0)
        size_t pass_count = 0;
        for (const auto& s : results.s_params) {
            if (s.vswr() < 2.0) {
                pass_count++;
            }
        }
        results.yield_rate = 100.0 * pass_count / results.num_samples;
    }
};

// Correlation matrix for component variations
class correlation_matrix {
public:
    correlation_matrix(size_t n) : m_size(n), m_matrix(n * n, 0.0) {
        // Initialize as identity matrix
        for (size_t i = 0; i < n; ++i) {
            m_matrix[i * n + i] = 1.0;
        }
    }
    
    void set_correlation(size_t i, size_t j, double correlation) {
        m_matrix[i * m_size + j] = correlation;
        m_matrix[j * m_size + i] = correlation;  // Symmetric
    }
    
    double get_correlation(size_t i, size_t j) const {
        return m_matrix[i * m_size + j];
    }
    
    // Generate correlated random variables
    std::vector<double> generate_correlated(const std::vector<double>& independent) const {
        // Simplified: just return independent for now
        // TODO: Implement Cholesky decomposition for proper correlation
        return independent;
    }
    
private:
    size_t m_size;
    std::vector<double> m_matrix;
};

} // namespace cascadix

#endif // INCLUDED_MONTE_CARLO_H