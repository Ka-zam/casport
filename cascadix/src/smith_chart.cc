#include "../include/smith_chart.h"
#include <cmath>
#include <algorithm>
#include <limits>

#ifndef PI
#define PI 3.14159265358979323846
#endif

namespace cascadix {

// Implementation of arc coefficient generation functions
// Most functions are already implemented inline in the header

// Enhanced arc generation with better numerical stability
arc_coefficients smith_chart::get_series_l_arc_enhanced(double l_min, double l_max, double frequency, 
                                                       double r_series, double q_factor) const {
    arc_coefficients arc;
    arc.type = arc_type::CONSTANT_RESISTANCE;
    arc.z0_system = z0_system;
    
    double omega = 2.0 * PI * frequency;
    
    // Include Q-factor effects if specified
    if (q_factor > 0) {
        double esr = omega * (l_min + l_max) * 0.5 / q_factor;  // Equivalent series resistance
        r_series += esr;
    }
    
    double x_min = omega * l_min;
    double x_max = omega * l_max;
    
    arc.coeffs[0] = r_series;  // Fixed resistance
    arc.coeffs[1] = x_min;     // Starting reactance
    arc.coeffs[2] = x_max;     // Ending reactance
    arc.coeffs[3] = omega;     // Angular frequency
    arc.coeffs[4] = q_factor;  // Q factor for loss modeling
    
    return arc;
}

arc_coefficients smith_chart::get_series_c_arc_enhanced(double c_min, double c_max, double frequency,
                                                       double r_series, double q_factor) const {
    arc_coefficients arc;
    arc.type = arc_type::CONSTANT_RESISTANCE;
    arc.z0_system = z0_system;
    
    double omega = 2.0 * PI * frequency;
    
    // Include Q-factor effects if specified
    if (q_factor > 0) {
        double avg_cap = (c_min + c_max) * 0.5;
        double esr = 1.0 / (omega * avg_cap * q_factor);  // Equivalent series resistance
        r_series += esr;
    }
    
    double x_min = -1.0 / (omega * c_max);  // Note: larger C = smaller |X|
    double x_max = -1.0 / (omega * c_min);
    
    arc.coeffs[0] = r_series;  // Fixed resistance
    arc.coeffs[1] = x_min;     // Starting reactance
    arc.coeffs[2] = x_max;     // Ending reactance
    arc.coeffs[3] = omega;     // Angular frequency
    arc.coeffs[4] = q_factor;  // Q factor for loss modeling
    
    return arc;
}

// Calculate circle intersection points for grid rendering
std::vector<complex> smith_chart::get_circle_intersections(
    const std::pair<complex, double>& circle1,
    const std::pair<complex, double>& circle2) const {
    
    std::vector<complex> intersections;
    
    complex c1 = circle1.first;
    complex c2 = circle2.first;
    double r1 = circle1.second;
    double r2 = circle2.second;
    
    double d = std::abs(c2 - c1);  // Distance between centers
    
    // No intersection if circles are too far apart or one inside the other
    if (d > r1 + r2 || d < std::abs(r1 - r2) || d == 0.0) {
        return intersections;
    }
    
    // Calculate intersection points
    double a = (r1 * r1 - r2 * r2 + d * d) / (2.0 * d);
    double h = std::sqrt(r1 * r1 - a * a);
    
    // Point on line between centers
    complex p = c1 + a * (c2 - c1) / d;
    
    // Perpendicular offset
    complex offset = complex(-h * (c2.imag() - c1.imag()) / d, h * (c2.real() - c1.real()) / d);
    
    intersections.push_back(p + offset);
    intersections.push_back(p - offset);
    
    return intersections;
}

// Generate Smith chart grid points
std::vector<complex> smith_chart::generate_grid_points(
    const std::vector<double>& resistance_values,
    const std::vector<double>& reactance_values,
    size_t points_per_circle) const {
    
    std::vector<complex> grid_points;
    
    // Generate constant resistance circles
    for (double r : resistance_values) {
        auto circle = get_constant_r_circle(r);
        complex center = circle.first;
        double radius = circle.second;
        
        for (size_t i = 0; i < points_per_circle; ++i) {
            double theta = 2.0 * PI * i / points_per_circle;
            complex point = center + radius * complex(std::cos(theta), std::sin(theta));
            
            // Only include points inside unit circle
            if (std::abs(point) <= 1.0) {
                grid_points.push_back(point);
            }
        }
    }
    
    // Generate constant reactance circles
    for (double x : reactance_values) {
        if (x == 0.0) continue;  // Skip x=0 line (real axis)
        
        auto circle = get_constant_x_circle(x);
        complex center = circle.first;
        double radius = circle.second;
        
        for (size_t i = 0; i < points_per_circle; ++i) {
            double theta = 2.0 * PI * i / points_per_circle;
            complex point = center + radius * complex(std::cos(theta), std::sin(theta));
            
            // Only include points inside unit circle
            if (std::abs(point) <= 1.0) {
                grid_points.push_back(point);
            }
        }
    }
    
    return grid_points;
}

// Calculate Smith chart bounds for a given impedance range
std::pair<complex, complex> smith_chart::calculate_bounds(
    const std::vector<complex>& impedances) const {
    
    if (impedances.empty()) {
        return {complex(-1, -1), complex(1, 1)};
    }
    
    double min_real = std::numeric_limits<double>::max();
    double max_real = std::numeric_limits<double>::lowest();
    double min_imag = std::numeric_limits<double>::max();
    double max_imag = std::numeric_limits<double>::lowest();
    
    for (const auto& z : impedances) {
        complex gamma = impedance_to_gamma(z, z0_system);
        
        min_real = std::min(min_real, gamma.real());
        max_real = std::max(max_real, gamma.real());
        min_imag = std::min(min_imag, gamma.imag());
        max_imag = std::max(max_imag, gamma.imag());
    }
    
    // Add some margin
    double margin = 0.1;
    min_real -= margin;
    max_real += margin;
    min_imag -= margin;
    max_imag += margin;
    
    // Clamp to unit circle
    min_real = std::max(min_real, -1.0);
    max_real = std::min(max_real, 1.0);
    min_imag = std::max(min_imag, -1.0);
    max_imag = std::min(max_imag, 1.0);
    
    return {complex(min_real, min_imag), complex(max_real, max_imag)};
}

// Convert between different Smith chart representations
complex smith_chart::convert_impedance_to_admittance_gamma(const complex& z_gamma) const {
    // Convert impedance reflection coefficient to admittance reflection coefficient
    return -z_gamma;  // Simple relation for admittance Smith chart
}

// Calculate VSWR contours
std::vector<complex> smith_chart::generate_vswr_contour(double vswr, size_t num_points) const {
    std::vector<complex> contour;
    
    double gamma_mag = (vswr - 1.0) / (vswr + 1.0);
    
    for (size_t i = 0; i < num_points; ++i) {
        double theta = 2.0 * PI * i / num_points;
        complex point = gamma_mag * complex(std::cos(theta), std::sin(theta));
        contour.push_back(point);
    }
    
    return contour;
}

// Calculate stability circles for active devices
std::pair<complex, double> smith_chart::calculate_input_stability_circle(
    const s_parameters& s_params) const {
    
    complex s11 = s_params.s11;
    complex s12 = s_params.s12;
    complex s21 = s_params.s21;
    complex s22 = s_params.s22;
    
    complex delta = s11 * s22 - s12 * s21;
    
    // Input stability circle
    complex c_center = std::conj(s11 - delta * std::conj(s22)) / 
                      (std::norm(s11) - std::norm(delta));
    
    double radius = std::abs(s12 * s21) / 
                   std::abs(std::norm(s11) - std::norm(delta));
    
    return {c_center, radius};
}

std::pair<complex, double> smith_chart::calculate_output_stability_circle(
    const s_parameters& s_params) const {
    
    complex s11 = s_params.s11;
    complex s12 = s_params.s12;
    complex s21 = s_params.s21;
    complex s22 = s_params.s22;
    
    complex delta = s11 * s22 - s12 * s21;
    
    // Output stability circle
    complex c_center = std::conj(s22 - delta * std::conj(s11)) / 
                      (std::norm(s22) - std::norm(delta));
    
    double radius = std::abs(s12 * s21) / 
                   std::abs(std::norm(s22) - std::norm(delta));
    
    return {c_center, radius};
}

// Calculate gain circles
std::pair<complex, double> smith_chart::calculate_gain_circle(
    const s_parameters& s_params, double gain_db, bool is_source) const {
    
    complex s11 = s_params.s11;
    complex s12 = s_params.s12;
    complex s21 = s_params.s21;
    complex s22 = s_params.s22;
    
    double gain_linear = std::pow(10.0, gain_db / 10.0);
    
    if (is_source) {
        // Source gain circle
        complex delta = s11 * s22 - s12 * s21;
        double k = (1.0 - std::norm(s11) - std::norm(s22) + std::norm(delta)) / 
                  (2.0 * std::abs(s12 * s21));
        
        if (k >= 1.0) {  // Unconditionally stable
            complex c_center = gain_linear * std::conj(s11) / 
                              (1.0 + gain_linear * (std::norm(s11) - std::norm(delta)));
            
            double radius = std::sqrt(1.0 - 2.0 * k * std::abs(s12 * s21) * gain_linear + 
                                    std::pow(std::abs(s12 * s21) * gain_linear, 2)) / 
                           std::abs(1.0 + gain_linear * (std::norm(s11) - std::norm(delta)));
            
            return {c_center, radius};
        }
    }
    
    // Default return for unsupported cases
    return {complex(0, 0), 0.0};
}

} // namespace cascadix