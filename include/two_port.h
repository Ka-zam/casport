#ifndef INCLUDED_TWO_PORT_H
#define INCLUDED_TWO_PORT_H

#include <complex>
#include <array>
#include <iostream>
#include <cmath>

namespace cascadix {

using complex = std::complex<double>;
using cx_matrix = std::array<complex, 4>;  // Row-major: [A, B, C, D]

// Forward declarations
struct s_parameters;
struct z_parameters;
struct y_parameters;

class two_port {
public:
    // Constructors
    two_port();  // Identity matrix
    two_port(const complex& a, const complex& b, const complex& c, const complex& d);
    two_port(const cx_matrix& abcd);
    
    // Copy and move constructors
    two_port(const two_port& other) = default;
    two_port(two_port&& other) = default;
    two_port& operator=(const two_port& other) = default;
    two_port& operator=(two_port&& other) = default;
    
    // Destructor
    virtual ~two_port() = default;
    
    // Matrix element access
    complex a() const { return m_abcd[0]; }
    complex b() const { return m_abcd[1]; }
    complex c() const { return m_abcd[2]; }
    complex d() const { return m_abcd[3]; }
    
    // Matrix operations
    complex determinant() const;
    bool is_reciprocal(double tolerance = 1e-10) const;
    bool is_symmetric(double tolerance = 1e-10) const;
    bool is_lossless(double tolerance = 1e-10) const;
    
    // Impedance calculations
    complex input_impedance(const complex& z_load) const;
    complex output_impedance(const complex& z_source) const;
    complex characteristic_impedance() const;  // For symmetric networks
    
    // Parameter conversions
    s_parameters to_s_parameters(double z0) const;
    s_parameters to_s_parameters(const complex& z0) const;
    z_parameters to_z_parameters() const;
    y_parameters to_y_parameters() const;
    
    // Static constructors from other parameter types
    static two_port from_s_parameters(const s_parameters& s_params, double z0);
    static two_port from_s_parameters(const s_parameters& s_params, const complex& z0);
    
    // Voltage and current transfer functions
    complex voltage_gain(const complex& z_load) const;
    complex current_gain(const complex& z_load) const;
    complex power_gain(const complex& z_source, const complex& z_load) const;
    
    // Operators
    two_port& operator*=(const two_port& other);      // In-place cascading
    
    // Utility functions
    void print(std::ostream& os = std::cout) const;
    cx_matrix get_matrix() const { return m_abcd; }
    
protected:
    cx_matrix m_abcd;  // ABCD matrix in row-major order
};

// Parameter structures
struct s_parameters {
    complex s11, s12, s21, s22;
    
    s_parameters() : s11(0), s12(0), s21(0), s22(0) {}
    s_parameters(const complex& s11, const complex& s12, 
                 const complex& s21, const complex& s22)
        : s11(s11), s12(s12), s21(s21), s22(s22) {}
    
    complex determinant() const { return s11 * s22 - s12 * s21; }
    double return_loss_db() const { return -20.0 * std::log10(std::abs(s11)); }
    double insertion_loss_db() const { return -20.0 * std::log10(std::abs(s21)); }
    double vswr() const { 
        double mag_s11 = std::abs(s11);
        return (1.0 + mag_s11) / (1.0 - mag_s11); 
    }
};

struct z_parameters {
    complex z11, z12, z21, z22;
    
    z_parameters() : z11(0), z12(0), z21(0), z22(0) {}
    z_parameters(const complex& z11, const complex& z12,
                 const complex& z21, const complex& z22)
        : z11(z11), z12(z12), z21(z21), z22(z22) {}
    
    complex determinant() const { return z11 * z22 - z12 * z21; }
};

struct y_parameters {
    complex y11, y12, y21, y22;
    
    y_parameters() : y11(0), y12(0), y21(0), y22(0) {}
    y_parameters(const complex& y11, const complex& y12,
                 const complex& y21, const complex& y22)
        : y11(y11), y12(y12), y21(y21), y22(y22) {}
    
    complex determinant() const { return y11 * y22 - y12 * y21; }
};

// Non-member operator for cascading
inline two_port operator*(const two_port& tp1, const two_port& tp2) {
    // Direct implementation to avoid circular calls
    complex a_new = tp1.a() * tp2.a() + tp1.b() * tp2.c();
    complex b_new = tp1.a() * tp2.b() + tp1.b() * tp2.d();
    complex c_new = tp1.c() * tp2.a() + tp1.d() * tp2.c();
    complex d_new = tp1.c() * tp2.b() + tp1.d() * tp2.d();
    
    return two_port(a_new, b_new, c_new, d_new);
}

// Stream output operator
inline std::ostream& operator<<(std::ostream& os, const two_port& tp) {
    tp.print(os);
    return os;
}

// Utility functions for creating identity and other special matrices
inline two_port identity_two_port() {
    return two_port(1.0, 0.0, 0.0, 1.0);
}

// Parallel combination (future implementation)
// two_port operator||(const two_port& tp1, const two_port& tp2);

} // namespace cascadix

#endif // INCLUDED_TWO_PORT_H