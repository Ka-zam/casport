#ifndef INCLUDED_SMITH_CHART_H
#define INCLUDED_SMITH_CHART_H

#include "two_port.h"
#include "components.h"
#include "component_sweep.h"
#include <vector>
#include <array>

namespace cascadix {

// Arc types for Smith chart visualization
enum class arc_type {
    CONSTANT_RESISTANCE,     // Series R-L or R-C
    CONSTANT_REACTANCE,      // Varying R with fixed X
    CONSTANT_CONDUCTANCE,    // Shunt G-L or G-C
    CONSTANT_SUSCEPTANCE,    // Varying G with fixed B
    TRANSMISSION_LINE,       // Rotation around specific center
    VSWR_CIRCLE,            // Constant VSWR circle
    CUSTOM                   // User-defined arc
};

// Coefficients for GPU shader arc generation
struct arc_coefficients {
    arc_type type;
    
    // Packed coefficients (interpretation depends on type)
    // For series L/C: [R_fixed, X_start, X_stop, omega, 0, 0, 0, 0]
    // For shunt L/C: [G_fixed, B_start, B_stop, omega, 0, 0, 0, 0]
    // For T-line: [z0_real, z0_imag, beta_l_start, beta_l_stop, z_load_real, z_load_imag, center_x, center_y]
    std::array<float, 8> coeffs;
    
    // Arc center for transmission lines (Smith chart coordinates)
    complex center;
    
    // System impedance for normalization
    float z0_system;
    
    arc_coefficients() : type(arc_type::CUSTOM), coeffs{}, center(0, 0), z0_system(50.0f) {}
};

// Smith chart calculator class
class smith_chart {
public:
    smith_chart(double z0_sys = 50.0) : z0_system(z0_sys) {}
    
    // Convert impedance to reflection coefficient (Smith chart coordinates)
    static complex impedance_to_gamma(const complex& z, double z0) {
        complex z_norm = z / z0;
        return (z_norm - 1.0) / (z_norm + 1.0);
    }
    
    // Convert reflection coefficient to impedance
    static complex gamma_to_impedance(const complex& gamma, double z0) {
        return z0 * (1.0 + gamma) / (1.0 - gamma);
    }
    
    // Convert admittance to reflection coefficient
    static complex admittance_to_gamma(const complex& y, double y0) {
        complex y_norm = y / y0;
        return (1.0 - y_norm) / (1.0 + y_norm);
    }
    
    // Calculate VSWR from reflection coefficient
    static double gamma_to_vswr(const complex& gamma) {
        double mag = std::abs(gamma);
        return (1.0 + mag) / (1.0 - mag);
    }
    
    // Get arc coefficients for series inductor
    arc_coefficients get_series_l_arc(double l_min, double l_max, double frequency, 
                                      double r_series = 0.0) const {
        arc_coefficients arc;
        arc.type = arc_type::CONSTANT_RESISTANCE;
        arc.z0_system = z0_system;
        
        double omega = 2.0 * PI * frequency;
        double x_min = omega * l_min;
        double x_max = omega * l_max;
        
        arc.coeffs[0] = r_series;  // Fixed resistance
        arc.coeffs[1] = x_min;     // Starting reactance
        arc.coeffs[2] = x_max;     // Ending reactance
        arc.coeffs[3] = omega;     // Angular frequency
        
        return arc;
    }
    
    // Get arc coefficients for series capacitor
    arc_coefficients get_series_c_arc(double c_min, double c_max, double frequency,
                                      double r_series = 0.0) const {
        arc_coefficients arc;
        arc.type = arc_type::CONSTANT_RESISTANCE;
        arc.z0_system = z0_system;
        
        double omega = 2.0 * PI * frequency;
        double x_min = -1.0 / (omega * c_max);  // Note: larger C = smaller |X|
        double x_max = -1.0 / (omega * c_min);
        
        arc.coeffs[0] = r_series;  // Fixed resistance
        arc.coeffs[1] = x_min;     // Starting reactance
        arc.coeffs[2] = x_max;     // Ending reactance
        arc.coeffs[3] = omega;     // Angular frequency
        
        return arc;
    }
    
    // Get arc coefficients for shunt inductor
    arc_coefficients get_shunt_l_arc(double l_min, double l_max, double frequency,
                                     double g_shunt = 0.0) const {
        arc_coefficients arc;
        arc.type = arc_type::CONSTANT_CONDUCTANCE;
        arc.z0_system = z0_system;
        
        double omega = 2.0 * PI * frequency;
        double b_min = -1.0 / (omega * l_max);  // Susceptance
        double b_max = -1.0 / (omega * l_min);
        
        arc.coeffs[0] = g_shunt;   // Fixed conductance
        arc.coeffs[1] = b_min;     // Starting susceptance
        arc.coeffs[2] = b_max;     // Ending susceptance
        arc.coeffs[3] = omega;     // Angular frequency
        
        return arc;
    }
    
    // Get arc coefficients for shunt capacitor
    arc_coefficients get_shunt_c_arc(double c_min, double c_max, double frequency,
                                     double g_shunt = 0.0) const {
        arc_coefficients arc;
        arc.type = arc_type::CONSTANT_CONDUCTANCE;
        arc.z0_system = z0_system;
        
        double omega = 2.0 * PI * frequency;
        double b_min = omega * c_min;  // Susceptance
        double b_max = omega * c_max;
        
        arc.coeffs[0] = g_shunt;   // Fixed conductance
        arc.coeffs[1] = b_min;     // Starting susceptance
        arc.coeffs[2] = b_max;     // Ending susceptance
        arc.coeffs[3] = omega;     // Angular frequency
        
        return arc;
    }
    
    // Get arc coefficients for transmission line (with complex Z0 support)
    arc_coefficients get_tline_arc(const complex& z0_line, double length_min, double length_max,
                                   double frequency, const complex& z_load,
                                   double alpha = 0.0) const {
        arc_coefficients arc;
        arc.type = arc_type::TRANSMISSION_LINE;
        arc.z0_system = z0_system;
        
        // Calculate center of rotation (normalized Z0 point)
        complex z0_norm = z0_line / z0_system;
        arc.center = impedance_to_gamma(z0_norm * z0_system, z0_system);
        
        // Phase constant
        double beta = 2.0 * PI * frequency / C0;  // Assuming air/vacuum
        
        arc.coeffs[0] = z0_line.real();          // Z0 real part
        arc.coeffs[1] = z0_line.imag();          // Z0 imaginary part
        arc.coeffs[2] = beta * length_min;       // Starting phase
        arc.coeffs[3] = beta * length_max;       // Ending phase
        arc.coeffs[4] = z_load.real();           // Load impedance real
        arc.coeffs[5] = z_load.imag();           // Load impedance imag
        arc.coeffs[6] = arc.center.real();       // Center X coordinate
        arc.coeffs[7] = arc.center.imag();       // Center Y coordinate
        
        return arc;
    }
    
    // Get VSWR circle coefficients
    arc_coefficients get_vswr_circle(double vswr) const {
        arc_coefficients arc;
        arc.type = arc_type::VSWR_CIRCLE;
        arc.z0_system = z0_system;
        
        // VSWR circle has center at origin and radius based on VSWR
        double gamma_mag = (vswr - 1.0) / (vswr + 1.0);
        
        arc.coeffs[0] = 0.0;        // Center X (origin)
        arc.coeffs[1] = 0.0;        // Center Y (origin)
        arc.coeffs[2] = gamma_mag;  // Radius
        arc.coeffs[3] = vswr;       // VSWR value
        
        arc.center = complex(0, 0);
        
        return arc;
    }
    
    // Convert component sweep results to arc coefficients
    arc_coefficients component_sweep_to_arc(const component_sweep& sweep,
                                           const component_sweep_results& results) const {
        arc_coefficients arc;
        arc.z0_system = z0_system;
        
        // Determine arc type based on component
        switch (sweep.type) {
            case component_type::SERIES_L:
            case component_type::SERIES_C:
            case component_type::SERIES_R:
                arc.type = arc_type::CONSTANT_RESISTANCE;
                break;
            case component_type::SHUNT_L:
            case component_type::SHUNT_C:
            case component_type::SHUNT_R:
                arc.type = arc_type::CONSTANT_CONDUCTANCE;
                break;
            case component_type::TRANSMISSION_LINE:
                arc.type = arc_type::TRANSMISSION_LINE;
                break;
        }
        
        // Extract start and end points
        if (!results.impedances.empty()) {
            complex z_start = results.impedances.front();
            complex z_end = results.impedances.back();
            
            arc.coeffs[0] = z_start.real();
            arc.coeffs[1] = z_start.imag();
            arc.coeffs[2] = z_end.real();
            arc.coeffs[3] = z_end.imag();
            arc.coeffs[4] = sweep.frequency;
        }
        
        return arc;
    }
    
    // Calculate constant resistance circle parameters
    std::pair<complex, double> get_constant_r_circle(double r_normalized) const {
        // Center: (r/(r+1), 0)
        // Radius: 1/(r+1)
        complex center(r_normalized / (r_normalized + 1.0), 0.0);
        double radius = 1.0 / (r_normalized + 1.0);
        return {center, radius};
    }
    
    // Calculate constant reactance circle parameters
    std::pair<complex, double> get_constant_x_circle(double x_normalized) const {
        // Center: (1, 1/x)
        // Radius: |1/x|
        complex center(1.0, 1.0 / x_normalized);
        double radius = std::abs(1.0 / x_normalized);
        return {center, radius};
    }
    
    // Calculate constant conductance circle parameters
    std::pair<complex, double> get_constant_g_circle(double g_normalized) const {
        // Same as constant resistance but in admittance plane
        complex center(-g_normalized / (g_normalized + 1.0), 0.0);
        double radius = 1.0 / (g_normalized + 1.0);
        return {center, radius};
    }
    
    // Calculate constant susceptance circle parameters
    std::pair<complex, double> get_constant_b_circle(double b_normalized) const {
        // Same as constant reactance but in admittance plane
        complex center(-1.0, -1.0 / b_normalized);
        double radius = std::abs(1.0 / b_normalized);
        return {center, radius};
    }
    
    // Enhanced functions declared but implemented in .cc file
    arc_coefficients get_series_l_arc_enhanced(double l_min, double l_max, double frequency, 
                                               double r_series = 0.0, double q_factor = 0.0) const;
    arc_coefficients get_series_c_arc_enhanced(double c_min, double c_max, double frequency,
                                               double r_series = 0.0, double q_factor = 0.0) const;
    
    std::vector<complex> get_circle_intersections(
        const std::pair<complex, double>& circle1,
        const std::pair<complex, double>& circle2) const;
    
    std::vector<complex> generate_grid_points(
        const std::vector<double>& resistance_values,
        const std::vector<double>& reactance_values,
        size_t points_per_circle = 50) const;
    
    std::pair<complex, complex> calculate_bounds(
        const std::vector<complex>& impedances) const;
    
    complex convert_impedance_to_admittance_gamma(const complex& z_gamma) const;
    
    std::vector<complex> generate_vswr_contour(double vswr, size_t num_points = 100) const;
    
    std::pair<complex, double> calculate_input_stability_circle(const s_parameters& s_params) const;
    std::pair<complex, double> calculate_output_stability_circle(const s_parameters& s_params) const;
    std::pair<complex, double> calculate_gain_circle(const s_parameters& s_params, 
                                                    double gain_db, bool is_source) const;
    
private:
    double z0_system;  // System reference impedance
};

// Helper function to generate points along an arc for visualization
inline std::vector<complex> generate_arc_points(const arc_coefficients& arc, 
                                               size_t num_points = 100) {
    std::vector<complex> points;
    points.reserve(num_points);
    
    for (size_t i = 0; i < num_points; ++i) {
        double t = static_cast<double>(i) / (num_points - 1);
        
        switch (arc.type) {
            case arc_type::CONSTANT_RESISTANCE: {
                double r = arc.coeffs[0];
                double x = arc.coeffs[1] + t * (arc.coeffs[2] - arc.coeffs[1]);
                complex z(r, x);
                points.push_back(smith_chart::impedance_to_gamma(z, arc.z0_system));
                break;
            }
            case arc_type::CONSTANT_CONDUCTANCE: {
                double g = arc.coeffs[0];
                double b = arc.coeffs[1] + t * (arc.coeffs[2] - arc.coeffs[1]);
                complex y(g, b);
                double y0 = 1.0 / arc.z0_system;
                points.push_back(smith_chart::admittance_to_gamma(y, y0));
                break;
            }
            case arc_type::TRANSMISSION_LINE: {
                // Complex transmission line calculation
                complex z0_line(arc.coeffs[0], arc.coeffs[1]);
                double beta_l = arc.coeffs[2] + t * (arc.coeffs[3] - arc.coeffs[2]);
                complex z_load(arc.coeffs[4], arc.coeffs[5]);
                
                // Transmission line equation
                complex tan_bl(0, std::tan(beta_l));
                complex z_in = z0_line * (z_load + z0_line * tan_bl) / 
                                        (z0_line + z_load * tan_bl);
                
                points.push_back(smith_chart::impedance_to_gamma(z_in, arc.z0_system));
                break;
            }
            case arc_type::VSWR_CIRCLE: {
                // Generate circle points
                double theta = 2.0 * PI * t;
                double radius = arc.coeffs[2];
                complex point(radius * std::cos(theta), radius * std::sin(theta));
                points.push_back(point);
                break;
            }
            default:
                break;
        }
    }
    
    return points;
}

} // namespace cascadix

#endif // INCLUDED_SMITH_CHART_H