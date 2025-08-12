#include "../include/components.h"

namespace cascadix {

// Most implementations are inline in the header file for performance.
// This file is provided for any future non-inline implementations or
// specialized component functions that may be too complex for header inclusion.

// Example: Could add factory functions for common filters here
two_port make_butterworth_lc_lowpass_3rd(double cutoff_freq, double z0) {
    // Normalized Butterworth values for 3rd order: L1=L3=0.7654, C2=1.8478
    double omega_c = 2.0 * PI * cutoff_freq;
    
    double l1_value = 0.7654 * z0 / omega_c;
    double c2_value = 1.8478 / (z0 * omega_c);
    double l3_value = 0.7654 * z0 / omega_c;
    
    two_port l1 = series_inductor(l1_value, cutoff_freq);
    two_port c2 = shunt_capacitor(c2_value, cutoff_freq);
    two_port l3 = series_inductor(l3_value, cutoff_freq);
    
    return l1 * c2 * l3;
}

// Example: Pi attenuator
two_port make_pi_attenuator(double attenuation_db, double z0) {
    // Calculate resistor values for Pi attenuator
    double k = std::pow(10.0, attenuation_db / 20.0);
    
    double r_series = z0 * (k * k - 1.0) / (2.0 * k);
    double r_shunt = z0 * (k + 1.0) / (k - 1.0);
    
    two_port r1 = shunt_resistor(r_shunt);
    two_port r2 = series_resistor(r_series);
    two_port r3 = shunt_resistor(r_shunt);
    
    return r1 * r2 * r3;
}

// Example: T attenuator
two_port make_t_attenuator(double attenuation_db, double z0) {
    // Calculate resistor values for T attenuator
    double k = std::pow(10.0, attenuation_db / 20.0);
    
    double r_series = z0 * (k - 1.0) / (k + 1.0);
    double r_shunt = 2.0 * z0 * k / (k * k - 1.0);
    
    two_port r1 = series_resistor(r_series);
    two_port r2 = shunt_resistor(r_shunt);
    two_port r3 = series_resistor(r_series);
    
    return r1 * r2 * r3;
}

// Example: L-section impedance matching network
two_port make_l_match(double z_source, double z_load, double freq, bool highpass) {
    // Calculate L and C values for L-match
    // This assumes z_source < z_load and creates a lowpass structure
    // For highpass, swap L and C positions
    
    if (z_source >= z_load) {
        // Swap source and load for proper calculation
        std::swap(z_source, z_load);
    }
    
    double q = std::sqrt(z_load / z_source - 1.0);
    double omega = 2.0 * PI * freq;
    
    if (highpass) {
        // Highpass configuration: series C, shunt L
        double c_value = 1.0 / (omega * z_source * q);
        double l_value = z_load / (omega * q);
        
        two_port c = series_capacitor(c_value, freq);
        two_port l = shunt_inductor(l_value, freq);
        
        return c * l;
    } else {
        // Lowpass configuration: series L, shunt C
        double l_value = z_source * q / omega;
        double c_value = q / (omega * z_load);
        
        two_port l = series_inductor(l_value, freq);
        two_port c = shunt_capacitor(c_value, freq);
        
        return l * c;
    }
}

} // namespace cascadix