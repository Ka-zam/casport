#include "../include/component_sweep.h"
#include <cmath>
#include <stdexcept>

#ifndef PI
#define PI 3.14159265358979323846
#endif

namespace cascadix {

// Implementation is primarily in the header file for component_sweep struct
// Additional utility functions can be added here

// Validate component sweep parameters
bool validate_component_sweep(const component_sweep& sweep) {
    if (sweep.num_points < 2) {
        return false;
    }
    if (sweep.value_start == sweep.value_stop && sweep.num_points > 1) {
        return false;
    }
    if (sweep.frequency <= 0.0) {
        return false;
    }
    
    // Check for valid ranges based on component type
    switch (sweep.type) {
        case component_type::SERIES_R:
        case component_type::SHUNT_R:
            if (sweep.value_start < 0.0 || sweep.value_stop < 0.0) {
                return false;
            }
            break;
        case component_type::SERIES_L:
        case component_type::SHUNT_L:
            if (sweep.value_start <= 0.0 || sweep.value_stop <= 0.0) {
                return false;
            }
            break;
        case component_type::SERIES_C:
        case component_type::SHUNT_C:
            if (sweep.value_start <= 0.0 || sweep.value_stop <= 0.0) {
                return false;
            }
            break;
        case component_type::TRANSMISSION_LINE:
            if (sweep.value_start < 0.0 || sweep.value_stop < 0.0) {
                return false;
            }
            break;
        default:
            return false;
    }
    
    return true;
}

// Calculate optimal sweep points for visualization
size_t calculate_optimal_points(const component_sweep& sweep, double max_phase_change = 30.0) {
    // For reactive components, calculate based on frequency and reactance change
    double range = std::abs(sweep.value_stop - sweep.value_start);
    double frequency = sweep.frequency;
    double omega = 2.0 * PI * frequency;
    
    size_t optimal_points = sweep.num_points;
    
    switch (sweep.type) {
        case component_type::SERIES_L:
        case component_type::SHUNT_L: {
            // For inductors, X = ωL
            double x_range = omega * range;
            // More points needed for larger reactance changes
            optimal_points = std::max(static_cast<size_t>(x_range / 10.0), sweep.num_points);
            break;
        }
        case component_type::SERIES_C:
        case component_type::SHUNT_C: {
            // For capacitors, X = 1/(ωC)
            double c_min = std::min(sweep.value_start, sweep.value_stop);
            double c_max = std::max(sweep.value_start, sweep.value_stop);
            double x_range = std::abs(1.0/(omega * c_min) - 1.0/(omega * c_max));
            optimal_points = std::max(static_cast<size_t>(x_range / 10.0), sweep.num_points);
            break;
        }
        case component_type::TRANSMISSION_LINE: {
            // For transmission lines, phase change = βl
            double beta = omega / C0;  // Assume air/vacuum
            double phase_range = beta * range * 180.0 / PI;  // Convert to degrees
            optimal_points = std::max(static_cast<size_t>(phase_range / max_phase_change), sweep.num_points);
            break;
        }
        default:
            break;
    }
    
    // Cap at reasonable maximum
    return std::min(optimal_points, static_cast<size_t>(1000));
}

// Create component sweep with adaptive point count
component_sweep make_adaptive_component_sweep(
    component_type type,
    double value_start,
    double value_stop,
    double frequency,
    double max_phase_change) {
    
    component_sweep sweep(type, value_start, value_stop, 50, frequency);  // Start with 50 points
    size_t optimal_points = calculate_optimal_points(sweep, max_phase_change);
    sweep.num_points = optimal_points;
    
    return sweep;
}

// Perform component sweep with error checking
component_sweep_results perform_component_sweep_checked(
    const component_sweep& sweep,
    double z0_system,
    const two_port& cascade_before,
    const two_port& cascade_after,
    const complex& z_load) {
    
    if (!validate_component_sweep(sweep)) {
        throw std::invalid_argument("Invalid component sweep parameters");
    }
    
    return perform_component_sweep(sweep, z0_system, cascade_before, cascade_after, z_load);
}

// Calculate component value at specific Smith chart angle
double calculate_component_value_at_angle(
    const component_sweep& sweep,
    double target_angle_degrees,
    double z0_system,
    const two_port& cascade_before,
    const two_port& cascade_after,
    const complex& z_load) {
    
    // Binary search for the component value that gives the target angle
    double value_low = std::min(sweep.value_start, sweep.value_stop);
    double value_high = std::max(sweep.value_start, sweep.value_stop);
    double target_angle_rad = target_angle_degrees * PI / 180.0;
    
    const double tolerance = 1e-6;
    const int max_iterations = 50;
    
    for (int i = 0; i < max_iterations; ++i) {
        double value_mid = (value_low + value_high) * 0.5;
        
        // Create temporary sweep with single point
        component_sweep temp_sweep(sweep.type, value_mid, value_mid, 1, sweep.frequency);
        two_port component = temp_sweep.create_network(value_mid);
        two_port network = cascade_before * component * cascade_after;
        
        complex z_in = network.input_impedance(z_load);
        complex z_norm = z_in / z0_system;
        complex gamma = (z_norm - 1.0) / (z_norm + 1.0);
        
        double angle = std::arg(gamma);
        if (angle < 0) angle += 2.0 * PI;  // Normalize to [0, 2π]
        
        if (std::abs(angle - target_angle_rad) < tolerance) {
            return value_mid;
        }
        
        if (angle < target_angle_rad) {
            value_low = value_mid;
        } else {
            value_high = value_mid;
        }
    }
    
    // Return best approximation
    return (value_low + value_high) * 0.5;
}

} // namespace cascadix