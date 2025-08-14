#ifndef INCLUDED_SMITH_CHART_GENERATOR_H
#define INCLUDED_SMITH_CHART_GENERATOR_H

#include "two_port.h"
#include "frequency_sweep.h"
#include <vector>
#include <random>
#include <functional>

namespace cascadix {

// Configuration for Smith chart point generation
struct smith_chart_config {
    double min_spacing = 0.003;        // Minimum point spacing in Smith chart units
    double max_spacing = 0.015;        // Maximum point spacing in Smith chart units  
    double edge_boost_factor = 4.0;    // Density multiplier near Smith chart edge
    bool adaptive_sampling = true;     // Enable adaptive density sampling
    double edge_threshold = 0.7;       // |Gamma| threshold for considering "near edge"
    
    smith_chart_config() = default;
    smith_chart_config(double min_sp, double max_sp, double edge_boost = 4.0)
        : min_spacing(min_sp), max_spacing(max_sp), edge_boost_factor(edge_boost) {}
};

// Smith chart point generator with adaptive density
class smith_chart_generator {
public:
    explicit smith_chart_generator(const smith_chart_config& config = smith_chart_config());
    
    // Generate points from frequency sweep with adaptive density using network builder
    std::vector<float> generate_sweep_points(std::function<two_port(double)> network_builder,
                                            const frequency_sweep& frequencies,
                                            const complex& load_impedance,
                                            double z0_reference) const;
    
    // Generate points from frequency sweep with network builder (with real load)
    std::vector<float> generate_sweep_points(std::function<two_port(double)> network_builder,
                                            const frequency_sweep& frequencies,
                                            double load_impedance,
                                            double z0_reference) const;
    
    // Generate points from single network (single point)
    std::vector<float> generate_sweep_points(const two_port& network,
                                            const frequency_sweep& frequencies,
                                            const complex& load_impedance,
                                            double z0_reference) const;
    
    // Generate points from single network (with real load)
    std::vector<float> generate_sweep_points(const two_port& network,
                                            const frequency_sweep& frequencies,
                                            double load_impedance,
                                            double z0_reference) const;
    
    // Generate Monte Carlo point cloud
    std::vector<float> generate_monte_carlo_points(const std::vector<complex>& impedances,
                                                   double z0_reference) const;
    
    // Generate points from S-parameter data
    std::vector<float> generate_from_s11_data(const std::vector<complex>& s11_data,
                                              double z0_reference) const;
    
    // Direct impedance to Smith chart conversion
    std::vector<float> impedances_to_smith_points(const std::vector<complex>& impedances,
                                                  double z0_reference) const;
    
    // Configuration accessors
    void set_config(const smith_chart_config& config) { m_config = config; }
    const smith_chart_config& get_config() const { return m_config; }
    
    // Static utility functions
    static complex impedance_to_reflection(const complex& impedance, double z0);
    static complex reflection_to_impedance(const complex& reflection, double z0);
    static complex normalize_impedance(const complex& impedance, double z0);
    
    // Utility functions for testing and debugging
    double calculate_point_spacing(const complex& gamma) const;
    
private:
    smith_chart_config m_config;
    
    // Internal helper functions
    void interpolate_segment(const complex& gamma1, const complex& gamma2, 
                           std::vector<float>& points) const;
    
    bool should_interpolate(const complex& gamma1, const complex& gamma2) const;
    
    void add_point_to_vector(const complex& gamma, std::vector<float>& points) const;
    
    int calculate_interpolation_count(const complex& gamma1, const complex& gamma2) const;
};

// Monte Carlo sample generator for component variations
class monte_carlo_sampler {
public:
    struct component_variation {
        double nominal_value;
        double tolerance_percent;     // e.g., 5.0 for 5%
        enum Distribution { UNIFORM, GAUSSIAN } distribution = GAUSSIAN;
    };
    
    monte_carlo_sampler(unsigned int seed = 12345);
    
    // Generate samples for a single component
    std::vector<double> generate_samples(const component_variation& component,
                                       int num_samples) const;
    
    // Generate impedance samples for a network with varying components
    std::vector<complex> generate_impedance_samples(
        std::function<two_port(const std::vector<double>&)> network_builder,
        const std::vector<component_variation>& component_variations,
        int num_samples,
        double frequency,
        const complex& load_impedance) const;
    
private:
    mutable std::mt19937 m_rng;
    
    double sample_gaussian(double mean, double std_dev) const;
    double sample_uniform(double min_val, double max_val) const;
};

// Factory functions
inline std::vector<float> generate_network_sweep(const two_port& network,
                                                double start_freq, double stop_freq,
                                                int num_points, double z0,
                                                const smith_chart_config& config = smith_chart_config()) {
    frequency_sweep sweep(start_freq, stop_freq, num_points, sweep_type::LOG);
    smith_chart_generator generator(config);
    return generator.generate_sweep_points(network, sweep, z0, z0);
}

inline std::vector<float> generate_impedance_cloud(const std::vector<complex>& impedances,
                                                   double z0,
                                                   const smith_chart_config& config = smith_chart_config()) {
    smith_chart_generator generator(config);
    return generator.impedances_to_smith_points(impedances, z0);
}

} // namespace cascadix

#endif // INCLUDED_SMITH_CHART_GENERATOR_H