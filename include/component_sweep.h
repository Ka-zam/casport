#ifndef INCLUDED_COMPONENT_SWEEP_H
#define INCLUDED_COMPONENT_SWEEP_H

#include "two_port.h"
#include "components.h"
#include <vector>
#include <functional>

namespace cascadix {

// Component types for sweeping
enum class component_type {
    SERIES_R,
    SERIES_L,
    SERIES_C,
    SHUNT_R,
    SHUNT_L,
    SHUNT_C,
    TRANSMISSION_LINE
};

// Sweep configuration for component values
struct component_sweep {
    component_type type;
    double value_start;
    double value_stop;
    size_t num_points;
    double frequency;  // Operating frequency (fixed during sweep)
    sweep_type distribution;  // LINEAR or LOG
    
    component_sweep(component_type t, double start, double stop, size_t points, 
                   double freq, sweep_type dist = sweep_type::LINEAR)
        : type(t), value_start(start), value_stop(stop), 
          num_points(points), frequency(freq), distribution(dist) {}
    
    // Generate component values for sweep
    std::vector<double> get_values() const {
        std::vector<double> values;
        values.reserve(num_points);
        
        if (distribution == sweep_type::LINEAR) {
            double step = (value_stop - value_start) / (num_points - 1);
            for (size_t i = 0; i < num_points; ++i) {
                values.push_back(value_start + i * step);
            }
        } else { // LOG
            double log_start = std::log10(value_start);
            double log_stop = std::log10(value_stop);
            double log_step = (log_stop - log_start) / (num_points - 1);
            for (size_t i = 0; i < num_points; ++i) {
                values.push_back(std::pow(10.0, log_start + i * log_step));
            }
        }
        
        return values;
    }
    
    // Create two_port network for a given component value
    two_port create_network(double value) const {
        switch (type) {
            case component_type::SERIES_R:
                return series_resistor(value);
            case component_type::SERIES_L:
                return series_inductor(value, frequency);
            case component_type::SERIES_C:
                return series_capacitor(value, frequency);
            case component_type::SHUNT_R:
                return shunt_resistor(value);
            case component_type::SHUNT_L:
                return shunt_inductor(value, frequency);
            case component_type::SHUNT_C:
                return shunt_capacitor(value, frequency);
            case component_type::TRANSMISSION_LINE:
                // Value represents length in meters
                return transmission_line(value, 50.0, frequency);  // Default 50Ω
            default:
                return identity_two_port();
        }
    }
};

// Results from component value sweep
struct component_sweep_results {
    std::vector<double> values;  // Component values
    std::vector<complex> impedances;  // Resulting impedances
    std::vector<complex> admittances;  // Resulting admittances
    std::vector<s_parameters> s_params;  // S-parameters at each value
    std::vector<complex> reflection_coefficients;  // Gamma values
    
    // Get impedances normalized to system Z0
    std::vector<complex> get_normalized_impedances(double z0) const {
        std::vector<complex> z_norm;
        z_norm.reserve(impedances.size());
        for (const auto& z : impedances) {
            z_norm.push_back(z / z0);
        }
        return z_norm;
    }
    
    // Get Smith chart coordinates (reflection coefficients)
    std::vector<complex> get_smith_coordinates() const {
        return reflection_coefficients;
    }
};

// Perform component value sweep
inline component_sweep_results perform_component_sweep(
    const component_sweep& sweep,
    double z0_system = 50.0,
    const two_port& cascade_before = identity_two_port(),
    const two_port& cascade_after = identity_two_port(),
    const complex& z_load = complex(50.0, 0.0)) {
    
    component_sweep_results results;
    auto values = sweep.get_values();
    results.values = values;
    results.impedances.reserve(values.size());
    results.admittances.reserve(values.size());
    results.s_params.reserve(values.size());
    results.reflection_coefficients.reserve(values.size());
    
    for (double value : values) {
        // Create component network
        two_port component = sweep.create_network(value);
        
        // Cascade with before/after networks if provided
        two_port network = cascade_before * component * cascade_after;
        
        // Calculate parameters
        complex z_in = network.input_impedance(z_load);
        results.impedances.push_back(z_in);
        results.admittances.push_back(1.0 / z_in);
        
        // S-parameters
        auto s = network.to_s_parameters(z0_system);
        results.s_params.push_back(s);
        
        // Reflection coefficient (Smith chart coordinate)
        complex z_norm = z_in / z0_system;
        complex gamma = (z_norm - 1.0) / (z_norm + 1.0);
        results.reflection_coefficients.push_back(gamma);
    }
    
    return results;
}

// Calculate arc range for a component
struct arc_range {
    double value_min;
    double value_max;
    complex z_start;  // Starting impedance
    complex z_stop;   // Ending impedance
    complex gamma_start;  // Starting Smith chart position
    complex gamma_stop;   // Ending Smith chart position
};

// Calculate practical sweep range for visualization
inline arc_range calculate_arc_range(
    component_type type,
    double nominal_value,
    double frequency,
    double tolerance = 0.2,  // ±20% default
    double z0_system = 50.0) {
    
    arc_range range;
    range.value_min = nominal_value * (1.0 - tolerance);
    range.value_max = nominal_value * (1.0 + tolerance);
    
    // Create sweep with just endpoints
    component_sweep sweep(type, range.value_min, range.value_max, 2, frequency);
    
    // Calculate impedances at endpoints
    two_port net_min = sweep.create_network(range.value_min);
    two_port net_max = sweep.create_network(range.value_max);
    
    range.z_start = net_min.input_impedance(complex(z0_system, 0.0));
    range.z_stop = net_max.input_impedance(complex(z0_system, 0.0));
    
    // Convert to Smith chart coordinates
    complex z_norm_start = range.z_start / z0_system;
    complex z_norm_stop = range.z_stop / z0_system;
    
    range.gamma_start = (z_norm_start - 1.0) / (z_norm_start + 1.0);
    range.gamma_stop = (z_norm_stop - 1.0) / (z_norm_stop + 1.0);
    
    return range;
}

// Builder function for creating component sweep from string description
inline component_sweep make_component_sweep(
    const std::string& component_desc,
    double value_start,
    double value_stop,
    size_t num_points,
    double frequency) {
    
    component_type type;
    
    if (component_desc == "series_R") type = component_type::SERIES_R;
    else if (component_desc == "series_L") type = component_type::SERIES_L;
    else if (component_desc == "series_C") type = component_type::SERIES_C;
    else if (component_desc == "shunt_R") type = component_type::SHUNT_R;
    else if (component_desc == "shunt_L") type = component_type::SHUNT_L;
    else if (component_desc == "shunt_C") type = component_type::SHUNT_C;
    else if (component_desc == "tline") type = component_type::TRANSMISSION_LINE;
    else type = component_type::SERIES_R;  // Default
    
    return component_sweep(type, value_start, value_stop, num_points, frequency);
}

} // namespace cascadix

#endif // INCLUDED_COMPONENT_SWEEP_H