#include <emscripten/bind.h>
#include "../include/cascadix.h"

using namespace emscripten;
using namespace cascadix;

// Wrapper functions for complex number handling
struct complex_wrapper {
    double real;
    double imag;
    
    complex_wrapper() : real(0.0), imag(0.0) {}
    complex_wrapper(double r, double i) : real(r), imag(i) {}
    complex_wrapper(const complex& c) : real(c.real()), imag(c.imag()) {}
    
    complex to_complex() const { return complex(real, imag); }
};

// Wrapper for s_parameters
struct s_params_wrapper {
    complex_wrapper s11, s12, s21, s22;
    
    s_params_wrapper() : s11(), s12(), s21(), s22() {}
    s_params_wrapper(const s_parameters& s) 
        : s11(s.s11), s12(s.s12), s21(s.s21), s22(s.s22) {}
};

// Wrapper functions for two_port
complex_wrapper get_a(const two_port& tp) { return complex_wrapper(tp.a()); }
complex_wrapper get_b(const two_port& tp) { return complex_wrapper(tp.b()); }
complex_wrapper get_c(const two_port& tp) { return complex_wrapper(tp.c()); }
complex_wrapper get_d(const two_port& tp) { return complex_wrapper(tp.d()); }

complex_wrapper get_input_impedance(const two_port& tp, const complex_wrapper& z_load) {
    return complex_wrapper(tp.input_impedance(z_load.to_complex()));
}

s_params_wrapper get_s_parameters(const two_port& tp, double z0) {
    return s_params_wrapper(tp.to_s_parameters(z0));
}

// Factory functions that return two_port objects
two_port create_series_resistor(double r) {
    return series_resistor(r);
}

two_port create_series_inductor(double l, double freq) {
    return series_inductor(l, freq);
}

two_port create_series_capacitor(double c, double freq) {
    return series_capacitor(c, freq);
}

two_port create_shunt_resistor(double r) {
    return shunt_resistor(r);
}

two_port create_shunt_inductor(double l, double freq) {
    return shunt_inductor(l, freq);
}

two_port create_shunt_capacitor(double c, double freq) {
    return shunt_capacitor(c, freq);
}

two_port create_transmission_line(double length, double z0, double freq) {
    return transmission_line(length, z0, freq);
}

two_port create_quarter_wave_line(double z0, double freq) {
    return transmission_line::from_electrical_length(90.0, z0, freq);
}

two_port create_ideal_transformer(double turns_ratio) {
    return ideal_transformer(turns_ratio);
}

// Cascade operation
two_port cascade_networks(const two_port& tp1, const two_port& tp2) {
    return tp1 * tp2;
}

// Bindings
EMSCRIPTEN_BINDINGS(cascadix_module) {
    // Complex number wrapper
    value_object<complex_wrapper>("Complex")
        .field("real", &complex_wrapper::real)
        .field("imag", &complex_wrapper::imag);
    
    // S-parameters wrapper
    value_object<s_params_wrapper>("SParameters")
        .field("s11", &s_params_wrapper::s11)
        .field("s12", &s_params_wrapper::s12)
        .field("s21", &s_params_wrapper::s21)
        .field("s22", &s_params_wrapper::s22);
    
    // Two-port network class
    class_<two_port>("TwoPort")
        .constructor<>()
        .function("determinant", &two_port::determinant)
        .function("isReciprocal", &two_port::is_reciprocal)
        .function("isSymmetric", &two_port::is_symmetric)
        .function("isLossless", &two_port::is_lossless);
    
    // Property getters
    function("getA", &get_a);
    function("getB", &get_b);
    function("getC", &get_c);
    function("getD", &get_d);
    
    // Analysis functions
    function("getInputImpedance", &get_input_impedance);
    function("getSParameters", &get_s_parameters);
    
    // Component factory functions
    function("seriesResistor", &create_series_resistor);
    function("seriesInductor", &create_series_inductor);
    function("seriesCapacitor", &create_series_capacitor);
    function("shuntResistor", &create_shunt_resistor);
    function("shuntInductor", &create_shunt_inductor);
    function("shuntCapacitor", &create_shunt_capacitor);
    function("transmissionLine", &create_transmission_line);
    function("quarterWaveLine", &create_quarter_wave_line);
    function("idealTransformer", &create_ideal_transformer);
    
    // Operations
    function("cascade", &cascade_networks);
    
    // Utility functions
    function("butterworthLC3", &make_butterworth_lc_lowpass_3rd);
    function("piAttenuator", &make_pi_attenuator);
    function("tAttenuator", &make_t_attenuator);
    
    // Constants
    constant("PI", PI);
    constant("C0", C0);
}