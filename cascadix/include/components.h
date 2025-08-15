#ifndef INCLUDED_COMPONENTS_H
#define INCLUDED_COMPONENTS_H

#include "two_port.h"
#include <cmath>

namespace cascadix {

// Physical constants
inline constexpr double PI = 3.14159265358979323846;
inline constexpr double C0 = 299792458.0;  // Speed of light in m/s
inline constexpr double MU0 = 4.0 * PI * 1e-7;  // Permeability of free space
inline constexpr double EPS0 = 1.0 / (MU0 * C0 * C0);  // Permittivity of free space

// Base class for frequency-dependent components
class frequency_dependent_two_port : public two_port {
public:
    frequency_dependent_two_port(double freq) : m_frequency(freq), m_omega(2.0 * PI * freq) {}
    
    double frequency() const { return m_frequency; }
    double omega() const { return m_omega; }
    
protected:
    double m_frequency;  // Frequency in Hz
    double m_omega;      // Angular frequency in rad/s
};

// Series impedance element
class series_impedance : public two_port {
public:
    explicit series_impedance(const complex& z) 
        : two_port(1.0, z, 0.0, 1.0), m_impedance(z) {}
    
    complex impedance() const { return m_impedance; }
    
private:
    complex m_impedance;
};

// Shunt admittance element
class shunt_admittance : public two_port {
public:
    explicit shunt_admittance(const complex& y)
        : two_port(1.0, 0.0, y, 1.0), m_admittance(y) {}
    
    complex admittance() const { return m_admittance; }
    complex impedance() const { return 1.0 / m_admittance; }
    
private:
    complex m_admittance;
};

// Series resistor
class series_resistor : public series_impedance {
public:
    series_resistor(double r, double freq = 0.0)
        : series_impedance(complex(r, 0.0)), m_resistance(r) {}
    
    double resistance() const { return m_resistance; }
    
private:
    double m_resistance;
};

// Series inductor
class series_inductor : public frequency_dependent_two_port {
public:
    series_inductor(double l, double freq)
        : frequency_dependent_two_port(freq), m_inductance(l) {
        complex z(0.0, m_omega * m_inductance);
        m_abcd = {1.0, z, 0.0, 1.0};
    }
    
    double inductance() const { return m_inductance; }
    complex impedance() const { return complex(0.0, m_omega * m_inductance); }
    
private:
    double m_inductance;  // Inductance in Henries
};

// Series capacitor
class series_capacitor : public frequency_dependent_two_port {
public:
    series_capacitor(double c, double freq)
        : frequency_dependent_two_port(freq), m_capacitance(c) {
        complex z(0.0, -1.0 / (m_omega * m_capacitance));
        m_abcd = {1.0, z, 0.0, 1.0};
    }
    
    double capacitance() const { return m_capacitance; }
    complex impedance() const { return complex(0.0, -1.0 / (m_omega * m_capacitance)); }
    
private:
    double m_capacitance;  // Capacitance in Farads
};

// Shunt resistor
class shunt_resistor : public shunt_admittance {
public:
    shunt_resistor(double r, double freq = 0.0)
        : shunt_admittance(complex(1.0 / r, 0.0)), m_resistance(r) {}
    
    double resistance() const { return m_resistance; }
    
private:
    double m_resistance;
};

// Shunt inductor
class shunt_inductor : public frequency_dependent_two_port {
public:
    shunt_inductor(double l, double freq)
        : frequency_dependent_two_port(freq), m_inductance(l) {
        complex y(0.0, -1.0 / (m_omega * m_inductance));
        m_abcd = {1.0, 0.0, y, 1.0};
    }
    
    double inductance() const { return m_inductance; }
    complex admittance() const { return complex(0.0, -1.0 / (m_omega * m_inductance)); }
    
private:
    double m_inductance;
};

// Shunt capacitor
class shunt_capacitor : public frequency_dependent_two_port {
public:
    shunt_capacitor(double c, double freq)
        : frequency_dependent_two_port(freq), m_capacitance(c) {
        complex y(0.0, m_omega * m_capacitance);
        m_abcd = {1.0, 0.0, y, 1.0};
    }
    
    double capacitance() const { return m_capacitance; }
    complex admittance() const { return complex(0.0, m_omega * m_capacitance); }
    
private:
    double m_capacitance;
};

// Transmission line with complex Z0 support
class transmission_line : public frequency_dependent_two_port {
public:
    // Constructor with complex characteristic impedance (general case)
    transmission_line(double length, const complex& z0_complex, double freq,
                     double vf = 1.0, double alpha_np_per_m = 0.0)
        : frequency_dependent_two_port(freq),
          m_length(length),
          m_z0(z0_complex),
          m_velocity_factor(vf),
          m_alpha(alpha_np_per_m) {
        
        // Calculate propagation constant
        double beta = m_omega * std::sqrt(MU0 * EPS0) / m_velocity_factor;
        complex gamma(m_alpha, beta);
        complex gamma_l = gamma * m_length;
        
        // Calculate ABCD parameters for complex Z0
        complex cosh_gl = std::cosh(gamma_l);
        complex sinh_gl = std::sinh(gamma_l);
        
        m_abcd = {
            cosh_gl,
            m_z0 * sinh_gl,
            sinh_gl / m_z0,
            cosh_gl
        };
    }
    
    // Constructor with real characteristic impedance (convenience)
    transmission_line(double length, double z0_real, double freq, 
                     double vf = 1.0, double loss_db_per_m = 0.0)
        : transmission_line(length, complex(z0_real, 0.0), freq, vf, 
                           loss_db_per_m * std::log(10.0) / 20.0) {}
    
    // Constructor with electrical length in degrees
    static transmission_line from_electrical_length(double theta_degrees, double z0, double freq,
                                                   double vf = 1.0) {
        double wavelength = C0 / (freq * vf);
        double length = (theta_degrees / 360.0) * wavelength;
        return transmission_line(length, z0, freq, vf);
    }
    
    // Constructor for lossy line with complex Z0
    static transmission_line lossy(double length, const complex& z0, double freq,
                                  double alpha_np_per_m, double vf = 1.0) {
        return transmission_line(length, z0, freq, vf, alpha_np_per_m);
    }
    
    double length() const { return m_length; }
    complex characteristic_impedance() const { return m_z0; }
    double velocity_factor() const { return m_velocity_factor; }
    double electrical_length_degrees() const {
        double wavelength = C0 / (m_frequency * m_velocity_factor);
        return (m_length / wavelength) * 360.0;
    }
    double attenuation() const { return m_alpha; }
    
private:
    double m_length;  // Physical length in meters
    complex m_z0;     // Complex characteristic impedance
    double m_velocity_factor;  // Velocity factor (1.0 for air/vacuum)
    double m_alpha;   // Attenuation constant in Nepers/meter
};

// Series open stub (transmission line with open termination in series)
class series_open_stub : public frequency_dependent_two_port {
public:
    series_open_stub(double length, double z0, double freq, double vf = 1.0)
        : frequency_dependent_two_port(freq), m_length(length), m_z0(z0), m_vf(vf) {
        // Open circuit termination: Z_load = infinity
        // Input impedance of open-ended transmission line: Z_in = -j*Z0*cot(βl)
        double beta = m_omega * std::sqrt(MU0 * EPS0) / m_vf;
        double beta_l = beta * m_length;
        complex z_in(0.0, -m_z0 / std::tan(beta_l));  // -j*Z0*cot(βl)
        m_abcd = {1.0, z_in, 0.0, 1.0};  // Series impedance matrix
    }
    
    double length() const { return m_length; }
    double characteristic_impedance() const { return m_z0; }
    double velocity_factor() const { return m_vf; }
    complex input_impedance() const {
        double beta = m_omega * std::sqrt(MU0 * EPS0) / m_vf;
        double beta_l = beta * m_length;
        return complex(0.0, -m_z0 / std::tan(beta_l));
    }
    
private:
    double m_length;
    double m_z0;
    double m_vf;
};

// Series short stub (transmission line with short termination in series)
class series_short_stub : public frequency_dependent_two_port {
public:
    series_short_stub(double length, double z0, double freq, double vf = 1.0)
        : frequency_dependent_two_port(freq), m_length(length), m_z0(z0), m_vf(vf) {
        // Short circuit termination: Z_load = 0
        // Input impedance of short-ended transmission line: Z_in = j*Z0*tan(βl)
        double beta = m_omega * std::sqrt(MU0 * EPS0) / m_vf;
        double beta_l = beta * m_length;
        complex z_in(0.0, m_z0 * std::tan(beta_l));  // j*Z0*tan(βl)
        m_abcd = {1.0, z_in, 0.0, 1.0};  // Series impedance matrix
    }
    
    double length() const { return m_length; }
    double characteristic_impedance() const { return m_z0; }
    double velocity_factor() const { return m_vf; }
    complex input_impedance() const {
        double beta = m_omega * std::sqrt(MU0 * EPS0) / m_vf;
        double beta_l = beta * m_length;
        return complex(0.0, m_z0 * std::tan(beta_l));
    }
    
private:
    double m_length;
    double m_z0;
    double m_vf;
};

// Shunt open stub (transmission line with open termination to ground)
class shunt_open_stub : public frequency_dependent_two_port {
public:
    shunt_open_stub(double length, double z0, double freq, double vf = 1.0)
        : frequency_dependent_two_port(freq), m_length(length), m_z0(z0), m_vf(vf) {
        // Open circuit termination: Z_load = infinity
        // Input impedance: Z_in = -j*Z0*cot(βl)
        // Input admittance: Y_in = 1/Z_in = j*tan(βl)/Z0
        double beta = m_omega * std::sqrt(MU0 * EPS0) / m_vf;
        double beta_l = beta * m_length;
        complex y_in(0.0, std::tan(beta_l) / m_z0);  // j*tan(βl)/Z0
        m_abcd = {1.0, 0.0, y_in, 1.0};  // Shunt admittance matrix
    }
    
    double length() const { return m_length; }
    double characteristic_impedance() const { return m_z0; }
    double velocity_factor() const { return m_vf; }
    complex input_impedance() const {
        double beta = m_omega * std::sqrt(MU0 * EPS0) / m_vf;
        double beta_l = beta * m_length;
        return complex(0.0, -m_z0 / std::tan(beta_l));
    }
    complex input_admittance() const {
        double beta = m_omega * std::sqrt(MU0 * EPS0) / m_vf;
        double beta_l = beta * m_length;
        return complex(0.0, std::tan(beta_l) / m_z0);
    }
    
private:
    double m_length;
    double m_z0;
    double m_vf;
};

// Shunt short stub (transmission line with short termination to ground)
class shunt_short_stub : public frequency_dependent_two_port {
public:
    shunt_short_stub(double length, double z0, double freq, double vf = 1.0)
        : frequency_dependent_two_port(freq), m_length(length), m_z0(z0), m_vf(vf) {
        // Short circuit termination: Z_load = 0
        // Input impedance: Z_in = j*Z0*tan(βl)
        // Input admittance: Y_in = 1/Z_in = -j*cot(βl)/Z0
        double beta = m_omega * std::sqrt(MU0 * EPS0) / m_vf;
        double beta_l = beta * m_length;
        complex y_in(0.0, -1.0 / (m_z0 * std::tan(beta_l)));  // -j*cot(βl)/Z0
        m_abcd = {1.0, 0.0, y_in, 1.0};  // Shunt admittance matrix
    }
    
    double length() const { return m_length; }
    double characteristic_impedance() const { return m_z0; }
    double velocity_factor() const { return m_vf; }
    complex input_impedance() const {
        double beta = m_omega * std::sqrt(MU0 * EPS0) / m_vf;
        double beta_l = beta * m_length;
        return complex(0.0, m_z0 * std::tan(beta_l));
    }
    complex input_admittance() const {
        double beta = m_omega * std::sqrt(MU0 * EPS0) / m_vf;
        double beta_l = beta * m_length;
        return complex(0.0, -1.0 / (m_z0 * std::tan(beta_l)));
    }
    
private:
    double m_length;
    double m_z0;
    double m_vf;
};

// Shunt tee - creates a shunt connection from any two_port network
class shunt_tee : public two_port {
public:
    // Constructor with arbitrary termination impedance
    shunt_tee(const two_port& shunt_network, const complex& termination_impedance)
        : m_shunt_network(shunt_network), m_termination(termination_impedance) {
        // Calculate input impedance of the shunt network with given termination
        complex z_shunt = m_shunt_network.input_impedance(m_termination);
        
        // Convert to admittance (handle infinite impedance case)
        complex y_shunt;
        if (std::abs(z_shunt) < 1e-20) {
            y_shunt = complex(1e20, 0.0);  // Very high admittance for short circuit
        } else {
            y_shunt = 1.0 / z_shunt;
        }
        
        // Create ABCD matrix for shunt admittance
        m_abcd = {1.0, 0.0, y_shunt, 1.0};
    }
    
    // Convenience constructors
    static shunt_tee short_terminated(const two_port& network) {
        return shunt_tee(network, complex(0.0, 0.0));
    }
    
    static shunt_tee open_terminated(const two_port& network) {
        return shunt_tee(network, complex(1e12, 0.0));  // Very high impedance for open
    }
    
    static shunt_tee match_terminated(const two_port& network, double z0) {
        return shunt_tee(network, complex(z0, 0.0));
    }
    
    // Accessors
    const two_port& shunt_network() const { return m_shunt_network; }
    complex termination_impedance() const { return m_termination; }
    
    // Calculate the effective shunt impedance
    complex shunt_impedance() const {
        return m_shunt_network.input_impedance(m_termination);
    }
    
    complex shunt_admittance() const {
        complex z = shunt_impedance();
        if (std::abs(z) < 1e-20) {
            return complex(1e20, 0.0);
        }
        return 1.0 / z;
    }
    
private:
    two_port m_shunt_network;
    complex m_termination;
};

// Ideal transformer
class ideal_transformer : public two_port {
public:
    explicit ideal_transformer(double turns_ratio)
        : two_port(turns_ratio, 0.0, 0.0, 1.0 / turns_ratio),
          m_turns_ratio(turns_ratio) {}
    
    double turns_ratio() const { return m_turns_ratio; }
    double impedance_ratio() const { return m_turns_ratio * m_turns_ratio; }
    
private:
    double m_turns_ratio;  // n = N1/N2
};

// Series RLC (general impedance with R, L, C in series)
class series_rlc : public frequency_dependent_two_port {
public:
    series_rlc(double r, double l, double c, double freq)
        : frequency_dependent_two_port(freq), m_r(r), m_l(l), m_c(c) {
        complex z_l(0.0, m_omega * m_l);
        complex z_c(0.0, -1.0 / (m_omega * m_c));
        complex z_total = complex(m_r, 0.0) + z_l + z_c;
        m_abcd = {1.0, z_total, 0.0, 1.0};
    }
    
    complex impedance() const {
        return complex(m_r, m_omega * m_l - 1.0 / (m_omega * m_c));
    }
    
    double resonant_frequency() const {
        return 1.0 / (2.0 * PI * std::sqrt(m_l * m_c));
    }
    
    double q_factor() const {
        return (1.0 / m_r) * std::sqrt(m_l / m_c);
    }
    
private:
    double m_r, m_l, m_c;
};

// Shunt RLC (parallel RLC to ground)
class shunt_rlc : public frequency_dependent_two_port {
public:
    shunt_rlc(double r, double l, double c, double freq)
        : frequency_dependent_two_port(freq), m_r(r), m_l(l), m_c(c) {
        complex y_r(1.0 / m_r, 0.0);
        complex y_l(0.0, -1.0 / (m_omega * m_l));
        complex y_c(0.0, m_omega * m_c);
        complex y_total = y_r + y_l + y_c;
        m_abcd = {1.0, 0.0, y_total, 1.0};
    }
    
    complex admittance() const {
        return complex(1.0 / m_r, m_omega * m_c - 1.0 / (m_omega * m_l));
    }
    
    double resonant_frequency() const {
        return 1.0 / (2.0 * PI * std::sqrt(m_l * m_c));
    }
    
    double q_factor() const {
        return m_r * std::sqrt(m_c / m_l);
    }
    
private:
    double m_r, m_l, m_c;
};

// Factory functions for convenience
inline two_port make_series_r(double r) {
    return series_resistor(r);
}

inline two_port make_series_l(double l, double freq) {
    return series_inductor(l, freq);
}

inline two_port make_series_c(double c, double freq) {
    return series_capacitor(c, freq);
}

inline two_port make_shunt_r(double r) {
    return shunt_resistor(r);
}

inline two_port make_shunt_l(double l, double freq) {
    return shunt_inductor(l, freq);
}

inline two_port make_shunt_c(double c, double freq) {
    return shunt_capacitor(c, freq);
}

inline two_port make_tline(double length, double z0, double freq) {
    return transmission_line(length, z0, freq);
}

inline two_port make_quarter_wave_tline(double z0, double freq) {
    return transmission_line::from_electrical_length(90.0, z0, freq);
}

// Stub factory functions
inline two_port make_series_open_stub(double length, double z0, double freq, double vf = 1.0) {
    return series_open_stub(length, z0, freq, vf);
}

inline two_port make_series_short_stub(double length, double z0, double freq, double vf = 1.0) {
    return series_short_stub(length, z0, freq, vf);
}

inline two_port make_shunt_open_stub(double length, double z0, double freq, double vf = 1.0) {
    return shunt_open_stub(length, z0, freq, vf);
}

inline two_port make_shunt_short_stub(double length, double z0, double freq, double vf = 1.0) {
    return shunt_short_stub(length, z0, freq, vf);
}

// Quarter-wave stub variants
inline two_port make_quarter_wave_series_open_stub(double z0, double freq, double vf = 1.0) {
    double wavelength = C0 / (freq * vf);
    double length = wavelength / 4.0;
    return series_open_stub(length, z0, freq, vf);
}

inline two_port make_quarter_wave_series_short_stub(double z0, double freq, double vf = 1.0) {
    double wavelength = C0 / (freq * vf);
    double length = wavelength / 4.0;
    return series_short_stub(length, z0, freq, vf);
}

inline two_port make_quarter_wave_shunt_open_stub(double z0, double freq, double vf = 1.0) {
    double wavelength = C0 / (freq * vf);
    double length = wavelength / 4.0;
    return shunt_open_stub(length, z0, freq, vf);
}

inline two_port make_quarter_wave_shunt_short_stub(double z0, double freq, double vf = 1.0) {
    double wavelength = C0 / (freq * vf);
    double length = wavelength / 4.0;
    return shunt_short_stub(length, z0, freq, vf);
}

// Shunt tee factory functions
inline two_port make_shunt_tee(const two_port& network, const complex& termination = complex(0.0, 0.0)) {
    return shunt_tee(network, termination);
}

inline two_port make_shunt_tee_short(const two_port& network) {
    return shunt_tee::short_terminated(network);
}

inline two_port make_shunt_tee_open(const two_port& network) {
    return shunt_tee::open_terminated(network);
}

inline two_port make_shunt_tee_match(const two_port& network, double z0) {
    return shunt_tee::match_terminated(network, z0);
}

// Build shunt stubs from transmission lines using shunt_tee
inline two_port make_shunt_stub_from_tline(double length, double z0, double freq, 
                                          const complex& termination = complex(0.0, 0.0), 
                                          double vf = 1.0) {
    auto tline = transmission_line(length, z0, freq, vf);
    return shunt_tee(tline, termination);
}

// Convenience functions for common stub terminations using shunt_tee
inline two_port make_shunt_tee_short_stub(double length, double z0, double freq, double vf = 1.0) {
    auto tline = transmission_line(length, z0, freq, vf);
    return shunt_tee::short_terminated(tline);
}

inline two_port make_shunt_tee_open_stub(double length, double z0, double freq, double vf = 1.0) {
    auto tline = transmission_line(length, z0, freq, vf);
    return shunt_tee::open_terminated(tline);
}

} // namespace cascadix

#endif // INCLUDED_COMPONENTS_H