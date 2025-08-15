#ifndef INCLUDED_FREQUENCY_SWEEP_H
#define INCLUDED_FREQUENCY_SWEEP_H

#include "two_port.h"
#include "components.h"
#include <vector>
#include <functional>

namespace cascadix {

// Sweep types
enum class sweep_type {
    LINEAR,
    LOG
};

// Structure to hold frequency sweep parameters
struct frequency_sweep {
    double start_freq;
    double stop_freq;
    size_t num_points;
    sweep_type type;
    
    frequency_sweep(double start, double stop, size_t points, sweep_type t = sweep_type::LINEAR)
        : start_freq(start), stop_freq(stop), num_points(points), type(t) {}
    
    // Generate frequency points
    std::vector<double> get_frequencies() const {
        std::vector<double> frequencies;
        frequencies.reserve(num_points);
        
        if (type == sweep_type::LINEAR) {
            double step = (stop_freq - start_freq) / (num_points - 1);
            for (size_t i = 0; i < num_points; ++i) {
                frequencies.push_back(start_freq + i * step);
            }
        } else { // LOG
            double log_start = std::log10(start_freq);
            double log_stop = std::log10(stop_freq);
            double log_step = (log_stop - log_start) / (num_points - 1);
            for (size_t i = 0; i < num_points; ++i) {
                frequencies.push_back(std::pow(10.0, log_start + i * log_step));
            }
        }
        
        return frequencies;
    }
};

// Structure to hold sweep results
struct sweep_results {
    std::vector<double> frequencies;
    std::vector<s_parameters> s_params;
    std::vector<complex> input_impedances;
    std::vector<complex> output_impedances;
    
    // Convenience accessors
    std::vector<complex> get_s11() const {
        std::vector<complex> s11_values;
        s11_values.reserve(s_params.size());
        for (const auto& s : s_params) {
            s11_values.push_back(s.s11);
        }
        return s11_values;
    }
    
    std::vector<complex> get_s21() const {
        std::vector<complex> s21_values;
        s21_values.reserve(s_params.size());
        for (const auto& s : s_params) {
            s21_values.push_back(s.s21);
        }
        return s21_values;
    }
    
    std::vector<double> get_s11_db() const {
        std::vector<double> s11_db;
        s11_db.reserve(s_params.size());
        for (const auto& s : s_params) {
            s11_db.push_back(20.0 * std::log10(std::abs(s.s11)));
        }
        return s11_db;
    }
    
    std::vector<double> get_s21_db() const {
        std::vector<double> s21_db;
        s21_db.reserve(s_params.size());
        for (const auto& s : s_params) {
            s21_db.push_back(20.0 * std::log10(std::abs(s.s21)));
        }
        return s21_db;
    }
    
    std::vector<double> get_vswr() const {
        std::vector<double> vswr_values;
        vswr_values.reserve(s_params.size());
        for (const auto& s : s_params) {
            vswr_values.push_back(s.vswr());
        }
        return vswr_values;
    }
    
    std::vector<double> get_s11_phase_deg() const {
        std::vector<double> phases;
        phases.reserve(s_params.size());
        for (const auto& s : s_params) {
            phases.push_back(std::arg(s.s11) * 180.0 / PI);
        }
        return phases;
    }
    
    std::vector<double> get_s21_phase_deg() const {
        std::vector<double> phases;
        phases.reserve(s_params.size());
        for (const auto& s : s_params) {
            phases.push_back(std::arg(s.s21) * 180.0 / PI);
        }
        return phases;
    }
};

// Network builder function type - creates a network for a given frequency
using network_builder = std::function<two_port(double freq)>;

// Perform frequency sweep on a network builder function
inline sweep_results perform_sweep(const network_builder& builder, 
                                  const frequency_sweep& sweep,
                                  double z0 = 50.0,
                                  const complex& z_load = complex(50.0, 0.0),
                                  const complex& z_source = complex(50.0, 0.0)) {
    sweep_results results;
    results.frequencies = sweep.get_frequencies();
    results.s_params.reserve(results.frequencies.size());
    results.input_impedances.reserve(results.frequencies.size());
    results.output_impedances.reserve(results.frequencies.size());
    
    for (double freq : results.frequencies) {
        two_port network = builder(freq);
        results.s_params.push_back(network.to_s_parameters(z0));
        results.input_impedances.push_back(network.input_impedance(z_load));
        results.output_impedances.push_back(network.output_impedance(z_source));
    }
    
    return results;
}

// Convenience function for S-parameter sweep only
inline std::vector<s_parameters> sweep_s_parameters(const network_builder& builder,
                                                   const frequency_sweep& sweep,
                                                   double z0 = 50.0) {
    std::vector<s_parameters> s_params;
    auto frequencies = sweep.get_frequencies();
    s_params.reserve(frequencies.size());
    
    for (double freq : frequencies) {
        two_port network = builder(freq);
        s_params.push_back(network.to_s_parameters(z0));
    }
    
    return s_params;
}

// Example network builders for common circuits

// Butterworth filter builder
inline network_builder make_butterworth_builder(double cutoff_freq, double z0) {
    return [cutoff_freq, z0](double freq) {
        double omega_c = 2.0 * PI * cutoff_freq;
        double l1_value = 0.7654 * z0 / omega_c;
        double c2_value = 1.8478 / (z0 * omega_c);
        double l3_value = 0.7654 * z0 / omega_c;
        
        two_port l1 = series_inductor(l1_value, freq);
        two_port c2 = shunt_capacitor(c2_value, freq);
        two_port l3 = series_inductor(l3_value, freq);
        
        return l1 * c2 * l3;
    };
}

// L-match network builder
inline network_builder make_l_match_builder(double z_source, double z_load, bool highpass = false) {
    return [z_source, z_load, highpass](double freq) {
        double z_s = z_source;
        double z_l = z_load;
        
        if (z_s >= z_l) {
            std::swap(z_s, z_l);
        }
        
        double q = std::sqrt(z_l / z_s - 1.0);
        double omega = 2.0 * PI * freq;
        
        if (highpass) {
            double c_value = 1.0 / (omega * z_s * q);
            double l_value = z_l / (omega * q);
            
            two_port c = series_capacitor(c_value, freq);
            two_port l = shunt_inductor(l_value, freq);
            
            return c * l;
        } else {
            double l_value = z_s * q / omega;
            double c_value = q / (omega * z_l);
            
            two_port l = series_inductor(l_value, freq);
            two_port c = shunt_capacitor(c_value, freq);
            
            return l * c;
        }
    };
}

// Series RLC builder
inline network_builder make_series_rlc_builder(double r, double l, double c) {
    return [r, l, c](double freq) {
        return series_rlc(r, l, c, freq);
    };
}

// Transmission line builder
inline network_builder make_tline_builder(double length, double z0, double vf = 1.0, double loss_db_per_m = 0.0) {
    return [length, z0, vf, loss_db_per_m](double freq) {
        return transmission_line(length, z0, freq, vf, loss_db_per_m);
    };
}

// Cascade multiple network builders
inline network_builder cascade_builders(const std::vector<network_builder>& builders) {
    return [builders](double freq) {
        if (builders.empty()) {
            return identity_two_port();
        }
        
        two_port result = builders[0](freq);
        for (size_t i = 1; i < builders.size(); ++i) {
            result = result * builders[i](freq);
        }
        return result;
    };
}

} // namespace cascadix

#endif // INCLUDED_FREQUENCY_SWEEP_H