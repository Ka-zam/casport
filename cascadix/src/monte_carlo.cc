#include "../include/monte_carlo.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace cascadix {

// Enhanced Monte Carlo analyzer implementation
void monte_carlo_analyzer::calculate_statistics(monte_carlo_results& results) {
    if (results.impedances.empty()) return;
    
    // Calculate mean impedance
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
    
    // Calculate yield rate based on VSWR < 2.0 criterion
    size_t pass_count = 0;
    for (const auto& s : results.s_params) {
        if (s.vswr() < 2.0) {
            pass_count++;
        }
    }
    results.yield_rate = 100.0 * pass_count / results.num_samples;
}

// Advanced statistical analysis functions
double calculate_confidence_interval(const std::vector<double>& values, double confidence_level) {
    if (values.empty()) return 0.0;
    
    std::vector<double> sorted_values = values;
    std::sort(sorted_values.begin(), sorted_values.end());
    
    double alpha = 1.0 - confidence_level;
    size_t lower_idx = static_cast<size_t>(alpha * 0.5 * sorted_values.size());
    size_t upper_idx = static_cast<size_t>((1.0 - alpha * 0.5) * sorted_values.size());
    
    if (upper_idx >= sorted_values.size()) upper_idx = sorted_values.size() - 1;
    
    return sorted_values[upper_idx] - sorted_values[lower_idx];
}

// Histogram generation for visualization
std::vector<double> generate_histogram(const std::vector<double>& values, size_t num_bins) {
    if (values.empty()) return {};
    
    double min_val = *std::min_element(values.begin(), values.end());
    double max_val = *std::max_element(values.begin(), values.end());
    double bin_width = (max_val - min_val) / num_bins;
    
    std::vector<double> histogram(num_bins, 0.0);
    
    for (double value : values) {
        size_t bin = static_cast<size_t>((value - min_val) / bin_width);
        if (bin >= num_bins) bin = num_bins - 1;
        histogram[bin] += 1.0;
    }
    
    // Normalize to probabilities
    double total = static_cast<double>(values.size());
    for (double& count : histogram) {
        count /= total;
    }
    
    return histogram;
}

// Sensitivity analysis
struct sensitivity_result {
    size_t component_index;
    double sensitivity;
    double correlation;
};

std::vector<sensitivity_result> calculate_sensitivity_analysis(
    const monte_carlo_results& results) {
    
    std::vector<sensitivity_result> sensitivities;
    
    if (results.component_values.empty() || results.impedances.empty()) {
        return sensitivities;
    }
    
    size_t num_components = results.component_values[0].size();
    size_t num_samples = results.component_values.size();
    
    // Extract impedance magnitudes for analysis
    std::vector<double> z_magnitudes;
    z_magnitudes.reserve(num_samples);
    for (const auto& z : results.impedances) {
        z_magnitudes.push_back(std::abs(z));
    }
    
    double mean_z_mag = std::accumulate(z_magnitudes.begin(), z_magnitudes.end(), 0.0) / num_samples;
    
    for (size_t comp_idx = 0; comp_idx < num_components; ++comp_idx) {
        // Extract component values for this component
        std::vector<double> comp_values;
        comp_values.reserve(num_samples);
        for (size_t sample_idx = 0; sample_idx < num_samples; ++sample_idx) {
            comp_values.push_back(results.component_values[sample_idx][comp_idx]);
        }
        
        double mean_comp = std::accumulate(comp_values.begin(), comp_values.end(), 0.0) / num_samples;
        
        // Calculate correlation coefficient
        double numerator = 0.0;
        double denom_comp = 0.0;
        double denom_z = 0.0;
        
        for (size_t i = 0; i < num_samples; ++i) {
            double comp_dev = comp_values[i] - mean_comp;
            double z_dev = z_magnitudes[i] - mean_z_mag;
            
            numerator += comp_dev * z_dev;
            denom_comp += comp_dev * comp_dev;
            denom_z += z_dev * z_dev;
        }
        
        double correlation = numerator / std::sqrt(denom_comp * denom_z);
        
        // Calculate sensitivity (slope of linear regression)
        double sensitivity = numerator / denom_comp;
        
        sensitivities.push_back({comp_idx, sensitivity, correlation});
    }
    
    // Sort by absolute sensitivity
    std::sort(sensitivities.begin(), sensitivities.end(),
              [](const sensitivity_result& a, const sensitivity_result& b) {
                  return std::abs(a.sensitivity) > std::abs(b.sensitivity);
              });
    
    return sensitivities;
}

// Pareto front analysis for multi-objective optimization
struct pareto_point {
    std::vector<double> component_values;
    std::vector<double> objectives;  // e.g., [VSWR, insertion_loss, bandwidth]
    bool is_dominated = false;
};

std::vector<pareto_point> find_pareto_front(const std::vector<pareto_point>& points) {
    std::vector<pareto_point> pareto_front;
    
    for (size_t i = 0; i < points.size(); ++i) {
        bool is_dominated = false;
        
        for (size_t j = 0; j < points.size(); ++j) {
            if (i == j) continue;
            
            // Check if point j dominates point i
            bool j_dominates_i = true;
            bool j_strictly_better = false;
            
            for (size_t obj = 0; obj < points[i].objectives.size(); ++obj) {
                if (points[j].objectives[obj] > points[i].objectives[obj]) {
                    j_dominates_i = false;
                    break;
                }
                if (points[j].objectives[obj] < points[i].objectives[obj]) {
                    j_strictly_better = true;
                }
            }
            
            if (j_dominates_i && j_strictly_better) {
                is_dominated = true;
                break;
            }
        }
        
        if (!is_dominated) {
            pareto_front.push_back(points[i]);
        }
    }
    
    return pareto_front;
}

// Robust design analysis
double calculate_robustness_metric(const monte_carlo_results& results, 
                                  double nominal_performance,
                                  double performance_threshold) {
    if (results.s_params.empty()) return 0.0;
    
    size_t acceptable_count = 0;
    for (const auto& s : results.s_params) {
        double performance = s.vswr();  // Use VSWR as performance metric
        if (performance <= performance_threshold) {
            acceptable_count++;
        }
    }
    
    return static_cast<double>(acceptable_count) / results.num_samples;
}

} // namespace cascadix