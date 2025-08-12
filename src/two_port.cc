#include "../include/two_port.h"
#include <iomanip>
#include <stdexcept>

namespace cascadix {

// Default constructor - identity matrix
two_port::two_port() : m_abcd{1.0, 0.0, 0.0, 1.0} {}

// Constructor from individual elements
two_port::two_port(const complex& a, const complex& b, const complex& c, const complex& d)
    : m_abcd{a, b, c, d} {}

// Constructor from array
two_port::two_port(const cx_matrix& abcd) : m_abcd(abcd) {}

// Calculate determinant
complex two_port::determinant() const {
    return m_abcd[0] * m_abcd[3] - m_abcd[1] * m_abcd[2];
}

// Check if network is reciprocal (det = 1)
bool two_port::is_reciprocal(double tolerance) const {
    complex det = determinant();
    return std::abs(det - 1.0) < tolerance;
}

// Check if network is symmetric (A = D)
bool two_port::is_symmetric(double tolerance) const {
    return std::abs(m_abcd[0] - m_abcd[3]) < tolerance;
}

// Check if network is lossless (A and D real, B and C imaginary)
bool two_port::is_lossless(double tolerance) const {
    return (std::abs(m_abcd[0].imag()) < tolerance &&
            std::abs(m_abcd[3].imag()) < tolerance &&
            std::abs(m_abcd[1].real()) < tolerance &&
            std::abs(m_abcd[2].real()) < tolerance &&
            std::abs(determinant()) - 1.0 < tolerance);
}

// Calculate input impedance with given load
complex two_port::input_impedance(const complex& z_load) const {
    complex numerator = m_abcd[0] * z_load + m_abcd[1];
    complex denominator = m_abcd[2] * z_load + m_abcd[3];
    
    if (std::abs(denominator) < 1e-20) {
        throw std::runtime_error("Input impedance calculation: division by zero");
    }
    
    return numerator / denominator;
}

// Calculate output impedance with given source impedance
complex two_port::output_impedance(const complex& z_source) const {
    complex numerator = m_abcd[3] * z_source + m_abcd[1];
    complex denominator = m_abcd[2] * z_source + m_abcd[0];
    
    if (std::abs(denominator) < 1e-20) {
        throw std::runtime_error("Output impedance calculation: division by zero");
    }
    
    return numerator / denominator;
}

// Calculate characteristic impedance (for symmetric networks)
complex two_port::characteristic_impedance() const {
    if (!is_symmetric()) {
        throw std::runtime_error("Characteristic impedance only defined for symmetric networks");
    }
    
    if (std::abs(m_abcd[2]) < 1e-20) {
        throw std::runtime_error("Characteristic impedance: C parameter is zero");
    }
    
    return std::sqrt(m_abcd[1] / m_abcd[2]);
}

// Convert to S-parameters with real reference impedance
s_parameters two_port::to_s_parameters(double z0) const {
    return to_s_parameters(complex(z0, 0.0));
}

// Convert to S-parameters with complex reference impedance
s_parameters two_port::to_s_parameters(const complex& z0) const {
    complex a = m_abcd[0];
    complex b = m_abcd[1];
    complex c = m_abcd[2];
    complex d = m_abcd[3];
    
    complex denominator = a + b / z0 + c * z0 + d;
    
    if (std::abs(denominator) < 1e-20) {
        throw std::runtime_error("S-parameter conversion: division by zero");
    }
    
    complex s11 = (a + b / z0 - c * z0 - d) / denominator;
    complex s12 = 2.0 * determinant() / denominator;
    complex s21 = 2.0 / denominator;
    complex s22 = (-a + b / z0 - c * z0 + d) / denominator;
    
    return s_parameters(s11, s12, s21, s22);
}

// Convert to Z-parameters
z_parameters two_port::to_z_parameters() const {
    complex c = m_abcd[2];
    
    if (std::abs(c) < 1e-20) {
        throw std::runtime_error("Z-parameter conversion: C parameter is zero");
    }
    
    complex z11 = m_abcd[0] / c;
    complex z12 = determinant() / c;
    complex z21 = 1.0 / c;
    complex z22 = m_abcd[3] / c;
    
    return z_parameters(z11, z12, z21, z22);
}

// Convert to Y-parameters
y_parameters two_port::to_y_parameters() const {
    complex b = m_abcd[1];
    
    if (std::abs(b) < 1e-20) {
        throw std::runtime_error("Y-parameter conversion: B parameter is zero");
    }
    
    complex y11 = m_abcd[3] / b;
    complex y12 = -determinant() / b;
    complex y21 = -1.0 / b;
    complex y22 = m_abcd[0] / b;
    
    return y_parameters(y11, y12, y21, y22);
}

// Calculate voltage transfer function
complex two_port::voltage_gain(const complex& z_load) const {
    complex denominator = m_abcd[0] + m_abcd[1] / z_load;
    
    if (std::abs(denominator) < 1e-20) {
        throw std::runtime_error("Voltage gain calculation: division by zero");
    }
    
    return 1.0 / denominator;
}

// Calculate current transfer function
complex two_port::current_gain(const complex& z_load) const {
    complex denominator = m_abcd[2] * z_load + m_abcd[3];
    
    if (std::abs(denominator) < 1e-20) {
        throw std::runtime_error("Current gain calculation: division by zero");
    }
    
    return 1.0 / denominator;
}

// Calculate power gain
complex two_port::power_gain(const complex& z_source, const complex& z_load) const {
    // Power gain = |S21|^2 * (1 - |Gamma_S|^2) * (1 - |Gamma_L|^2) / |1 - S11*Gamma_S|^2 / |1 - S22*Gamma_L|^2
    // For simplicity, we'll calculate transducer gain with matched source and load
    complex vg = voltage_gain(z_load);
    complex z_in = input_impedance(z_load);
    
    // Voltage divider at input
    complex v1_over_vs = z_in / (z_source + z_in);
    
    // Total voltage gain
    complex total_vg = v1_over_vs * vg;
    
    // Power gain (assuming real impedances for simplicity)
    double pg = std::abs(total_vg) * std::abs(total_vg) * z_source.real() / z_load.real();
    
    return complex(pg, 0.0);
}

// In-place cascade operator
two_port& two_port::operator*=(const two_port& other) {
    // Direct implementation to avoid circular dependency
    complex a_new = m_abcd[0] * other.m_abcd[0] + m_abcd[1] * other.m_abcd[2];
    complex b_new = m_abcd[0] * other.m_abcd[1] + m_abcd[1] * other.m_abcd[3];
    complex c_new = m_abcd[2] * other.m_abcd[0] + m_abcd[3] * other.m_abcd[2];
    complex d_new = m_abcd[2] * other.m_abcd[1] + m_abcd[3] * other.m_abcd[3];
    
    m_abcd[0] = a_new;
    m_abcd[1] = b_new;
    m_abcd[2] = c_new;
    m_abcd[3] = d_new;
    
    return *this;
}

// Print matrix
void two_port::print(std::ostream& os) const {
    os << std::fixed << std::setprecision(6);
    os << "ABCD Matrix:\n";
    os << "[ " << std::setw(15) << m_abcd[0] << "  " << std::setw(15) << m_abcd[1] << " ]\n";
    os << "[ " << std::setw(15) << m_abcd[2] << "  " << std::setw(15) << m_abcd[3] << " ]\n";
    os << "Determinant: " << determinant() << "\n";
}

} // namespace cascadix