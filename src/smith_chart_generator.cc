#include "../include/smith_chart_generator.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <functional>

namespace cascadix {

// Constructor
smith_chart_generator::smith_chart_generator(const smith_chart_config& config)
    : m_config(config) {}

// Static utility: Convert impedance to reflection coefficient
complex smith_chart_generator::impedance_to_reflection(const complex& impedance, double z0) {
    complex z0_complex(z0, 0.0);
    return (impedance - z0_complex) / (impedance + z0_complex);
}

// Static utility: Convert reflection coefficient to impedance
complex smith_chart_generator::reflection_to_impedance(const complex& reflection, double z0) {
    complex z0_complex(z0, 0.0);
    return z0_complex * (1.0 + reflection) / (1.0 - reflection);
}

// Static utility: Normalize impedance
complex smith_chart_generator::normalize_impedance(const complex& impedance, double z0) {
    return impedance / z0;
}

// Generate points from frequency sweep using network builder
std::vector<float> smith_chart_generator::generate_sweep_points(
    std::function<two_port(double)> network_builder,
    const frequency_sweep& frequencies,
    const complex& load_impedance,
    double z0_reference) const {
    
    std::vector<float> points;
    points.reserve(frequencies.num_points * 2 * 4); // Reserve extra space for interpolation
    
    std::vector<double> freq_points = frequencies.get_frequencies();
    complex prev_gamma;
    bool first_point = true;
    
    for (size_t i = 0; i < freq_points.size(); i++) {
        double freq = freq_points[i];
        
        // Build network for this frequency
        two_port network = network_builder(freq);
        
        // Calculate input impedance at this frequency
        complex z_in = network.input_impedance(load_impedance);
        
        // Convert to Smith chart coordinates (reflection coefficient)
        complex gamma = impedance_to_reflection(z_in, z0_reference);
        
        // Add interpolated points if needed
        if (!first_point && m_config.adaptive_sampling && should_interpolate(prev_gamma, gamma)) {
            interpolate_segment(prev_gamma, gamma, points);
        }
        
        // Add the current point
        add_point_to_vector(gamma, points);
        
        prev_gamma = gamma;
        first_point = false;
    }
    
    return points;
}

// Generate points from frequency sweep with network builder (real load)
std::vector<float> smith_chart_generator::generate_sweep_points(
    std::function<two_port(double)> network_builder,
    const frequency_sweep& frequencies,
    double load_impedance,
    double z0_reference) const {
    
    return generate_sweep_points(network_builder, frequencies, complex(load_impedance, 0.0), z0_reference);
}

// Generate points from frequency sweep with single network (complex load)
std::vector<float> smith_chart_generator::generate_sweep_points(
    const two_port& network,
    const frequency_sweep& frequencies,
    const complex& load_impedance,
    double z0_reference) const {
    
    std::vector<float> points;
    
    // For a single network instance, we can't vary frequency
    // So we just calculate the impedance for the given load
    complex z_in = network.input_impedance(load_impedance);
    complex gamma = impedance_to_reflection(z_in, z0_reference);
    add_point_to_vector(gamma, points);
    
    return points;
}

// Generate points from frequency sweep with real load
std::vector<float> smith_chart_generator::generate_sweep_points(
    const two_port& network,
    const frequency_sweep& frequencies,
    double load_impedance,
    double z0_reference) const {
    
    return generate_sweep_points(network, frequencies, complex(load_impedance, 0.0), z0_reference);
}

// Generate Monte Carlo point cloud
std::vector<float> smith_chart_generator::generate_monte_carlo_points(
    const std::vector<complex>& impedances,
    double z0_reference) const {
    
    std::vector<float> points;
    points.reserve(impedances.size() * 2);
    
    for (const auto& impedance : impedances) {
        complex gamma = impedance_to_reflection(impedance, z0_reference);
        add_point_to_vector(gamma, points);
    }
    
    return points;
}

// Generate points from S11 data
std::vector<float> smith_chart_generator::generate_from_s11_data(
    const std::vector<complex>& s11_data,
    double z0_reference) const {
    
    std::vector<float> points;
    points.reserve(s11_data.size() * 2 * 4); // Extra space for interpolation
    
    complex prev_s11;
    bool first_point = true;
    
    for (const auto& s11 : s11_data) {
        // S11 is already a reflection coefficient, but verify it's normalized to our z0
        complex gamma = s11;  // Assume S11 data is already referenced to z0_reference
        
        // Add interpolated points if needed
        if (!first_point && m_config.adaptive_sampling && should_interpolate(prev_s11, gamma)) {
            interpolate_segment(prev_s11, gamma, points);
        }
        
        // Add the current point
        add_point_to_vector(gamma, points);
        
        prev_s11 = gamma;
        first_point = false;
    }
    
    return points;
}

// Direct impedance to Smith chart conversion
std::vector<float> smith_chart_generator::impedances_to_smith_points(
    const std::vector<complex>& impedances,
    double z0_reference) const {
    
    return generate_monte_carlo_points(impedances, z0_reference);
}

// Private helper: Calculate appropriate point spacing based on Smith chart radius
double smith_chart_generator::calculate_point_spacing(const complex& gamma) const {
    double radius = std::abs(gamma);
    
    if (radius < m_config.edge_threshold) {
        // Near center: use larger spacing
        double t = radius / m_config.edge_threshold;  // 0 to 1
        return m_config.max_spacing - t * (m_config.max_spacing - m_config.min_spacing);
    } else {
        // Near edge: use smaller spacing with boost factor
        double edge_factor = (radius - m_config.edge_threshold) / (1.0 - m_config.edge_threshold);
        return m_config.min_spacing / (1.0 + m_config.edge_boost_factor * edge_factor);
    }
}

// Private helper: Check if interpolation is needed between two points
bool smith_chart_generator::should_interpolate(const complex& gamma1, const complex& gamma2) const {
    double distance = std::abs(gamma2 - gamma1);
    
    // Calculate appropriate spacing based on both points
    double spacing1 = calculate_point_spacing(gamma1);
    double spacing2 = calculate_point_spacing(gamma2);
    double avg_spacing = (spacing1 + spacing2) * 0.5;
    
    return distance > avg_spacing;
}

// Private helper: Calculate how many interpolation points are needed
int smith_chart_generator::calculate_interpolation_count(const complex& gamma1, const complex& gamma2) const {
    double distance = std::abs(gamma2 - gamma1);
    double spacing1 = calculate_point_spacing(gamma1);
    double spacing2 = calculate_point_spacing(gamma2);
    double avg_spacing = (spacing1 + spacing2) * 0.5;
    
    int count = static_cast<int>(std::ceil(distance / avg_spacing)) - 1;
    return std::max(0, std::min(count, 20)); // Limit to prevent excessive points
}

// Private helper: Interpolate points between two Smith chart coordinates
void smith_chart_generator::interpolate_segment(const complex& gamma1, const complex& gamma2,
                                              std::vector<float>& points) const {
    
    int interp_count = calculate_interpolation_count(gamma1, gamma2);
    
    for (int i = 1; i <= interp_count; i++) {
        double t = double(i) / double(interp_count + 1);
        
        // Linear interpolation in Smith chart coordinates
        complex gamma_interp = gamma1 + t * (gamma2 - gamma1);
        
        add_point_to_vector(gamma_interp, points);
    }
}

// Private helper: Add a complex point to the float vector
void smith_chart_generator::add_point_to_vector(const complex& gamma, std::vector<float>& points) const {
    // Clamp to Smith chart bounds [-1, 1]
    double real_part = std::max(-1.0, std::min(1.0, gamma.real()));
    double imag_part = std::max(-1.0, std::min(1.0, gamma.imag()));
    
    points.push_back(static_cast<float>(real_part));
    points.push_back(static_cast<float>(imag_part));
}

// Monte Carlo sampler implementation
monte_carlo_sampler::monte_carlo_sampler(unsigned int seed) : m_rng(seed) {}

std::vector<double> monte_carlo_sampler::generate_samples(const component_variation& component,
                                                        int num_samples) const {
    std::vector<double> samples;
    samples.reserve(num_samples);
    
    double tolerance = component.nominal_value * component.tolerance_percent / 100.0;
    
    for (int i = 0; i < num_samples; i++) {
        double sample;
        
        if (component.distribution == component_variation::GAUSSIAN) {
            // 3-sigma = tolerance, so sigma = tolerance/3
            sample = sample_gaussian(component.nominal_value, tolerance / 3.0);
        } else {
            // Uniform distribution
            sample = sample_uniform(component.nominal_value - tolerance,
                                  component.nominal_value + tolerance);
        }
        
        // Ensure positive values for physical components
        sample = std::max(sample, component.nominal_value * 0.01);
        samples.push_back(sample);
    }
    
    return samples;
}

std::vector<complex> monte_carlo_sampler::generate_impedance_samples(
    std::function<two_port(const std::vector<double>&)> network_builder,
    const std::vector<component_variation>& component_variations,
    int num_samples,
    double frequency,
    const complex& load_impedance) const {
    
    std::vector<complex> impedances;
    impedances.reserve(num_samples);
    
    // Generate samples for each component
    std::vector<std::vector<double>> component_samples;
    for (const auto& variation : component_variations) {
        component_samples.push_back(generate_samples(variation, num_samples));
    }
    
    // Build network for each sample set
    for (int i = 0; i < num_samples; i++) {
        std::vector<double> sample_values;
        sample_values.reserve(component_variations.size());
        
        for (size_t j = 0; j < component_variations.size(); j++) {
            sample_values.push_back(component_samples[j][i]);
        }
        
        // Build the network with these component values
        two_port network = network_builder(sample_values);
        
        // Calculate input impedance
        complex z_in = network.input_impedance(load_impedance);
        impedances.push_back(z_in);
    }
    
    return impedances;
}

// Private helper: Sample from Gaussian distribution
double monte_carlo_sampler::sample_gaussian(double mean, double std_dev) const {
    static std::normal_distribution<double> dist(0.0, 1.0);
    return mean + std_dev * dist(m_rng);
}

// Private helper: Sample from uniform distribution  
double monte_carlo_sampler::sample_uniform(double min_val, double max_val) const {
    std::uniform_real_distribution<double> dist(min_val, max_val);
    return dist(m_rng);
}

} // namespace cascadix