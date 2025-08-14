#include <emscripten/bind.h>
#include "../include/cascadix.h"
#include "../include/smith_chart_generator.h"

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

// Stub functions
two_port create_series_open_stub(double length, double z0, double freq) {
    return series_open_stub(length, z0, freq);
}

two_port create_series_short_stub(double length, double z0, double freq) {
    return series_short_stub(length, z0, freq);
}

two_port create_shunt_open_stub(double length, double z0, double freq) {
    return shunt_open_stub(length, z0, freq);
}

two_port create_shunt_short_stub(double length, double z0, double freq) {
    return shunt_short_stub(length, z0, freq);
}

// Quarter-wave stubs
two_port create_quarter_wave_series_open_stub(double z0, double freq) {
    return make_quarter_wave_series_open_stub(z0, freq);
}

two_port create_quarter_wave_series_short_stub(double z0, double freq) {
    return make_quarter_wave_series_short_stub(z0, freq);
}

two_port create_quarter_wave_shunt_open_stub(double z0, double freq) {
    return make_quarter_wave_shunt_open_stub(z0, freq);
}

two_port create_quarter_wave_shunt_short_stub(double z0, double freq) {
    return make_quarter_wave_shunt_short_stub(z0, freq);
}

// S-parameters to ABCD conversion
two_port create_from_s_parameters(const s_params_wrapper& s, double z0) {
    s_parameters s_params(s.s11.to_complex(), s.s12.to_complex(),
                         s.s21.to_complex(), s.s22.to_complex());
    return two_port::from_s_parameters(s_params, z0);
}

// Shunt tee functions
two_port create_shunt_tee(const two_port& network, const complex_wrapper& termination) {
    return shunt_tee(network, termination.to_complex());
}

two_port create_shunt_tee_short(const two_port& network) {
    return shunt_tee::short_terminated(network);
}

two_port create_shunt_tee_open(const two_port& network) {
    return shunt_tee::open_terminated(network);
}

two_port create_shunt_tee_match(const two_port& network, double z0) {
    return shunt_tee::match_terminated(network, z0);
}

// Shunt stub from transmission line using shunt_tee
two_port create_shunt_stub_from_tline(double length, double z0, double freq, 
                                    const complex_wrapper& termination) {
    auto tline = transmission_line(length, z0, freq);
    return shunt_tee(tline, termination.to_complex());
}

two_port create_shunt_tee_short_stub(double length, double z0, double freq) {
    return make_shunt_tee_short_stub(length, z0, freq);
}

two_port create_shunt_tee_open_stub(double length, double z0, double freq) {
    return make_shunt_tee_open_stub(length, z0, freq);
}

// Smith chart point generation functions
emscripten::val generate_network_sweep_points(const two_port& network, 
                                            double start_freq, double stop_freq, 
                                            int num_points, double z0) {
    frequency_sweep sweep(start_freq, stop_freq, num_points, sweep_type::LOG);
    smith_chart_generator generator;
    auto points = generator.generate_sweep_points(network, sweep, z0, z0);
    
    return emscripten::val(emscripten::typed_memory_view(points.size(), points.data()));
}

emscripten::val generate_network_sweep_points_with_config(const two_port& network,
                                                        double start_freq, double stop_freq,
                                                        int num_points, double z0,
                                                        double min_spacing, double max_spacing,
                                                        double edge_boost) {
    frequency_sweep sweep(start_freq, stop_freq, num_points, sweep_type::LOG);
    smith_chart_config config(min_spacing, max_spacing, edge_boost);
    smith_chart_generator generator(config);
    auto points = generator.generate_sweep_points(network, sweep, z0, z0);
    
    return emscripten::val(emscripten::typed_memory_view(points.size(), points.data()));
}

emscripten::val impedances_to_smith_points(emscripten::val impedances_array, double z0) {
    // Convert JavaScript array to C++ vector
    std::vector<complex> impedances;
    int length = impedances_array["length"].as<int>();
    
    for (int i = 0; i < length; i += 2) {
        double real_part = impedances_array[i].as<double>();
        double imag_part = impedances_array[i + 1].as<double>();
        impedances.emplace_back(real_part, imag_part);
    }
    
    smith_chart_generator generator;
    auto points = generator.impedances_to_smith_points(impedances, z0);
    
    return emscripten::val(emscripten::typed_memory_view(points.size(), points.data()));
}

emscripten::val generate_monte_carlo_samples(std::function<two_port(emscripten::val)> network_builder,
                                           emscripten::val component_values,
                                           emscripten::val tolerances,
                                           int num_samples,
                                           double frequency,
                                           double load_impedance,
                                           double z0) {
    // Convert JavaScript arrays
    std::vector<double> nominal_values;
    std::vector<double> tolerance_percents;
    
    int num_components = component_values["length"].as<int>();
    for (int i = 0; i < num_components; i++) {
        nominal_values.push_back(component_values[i].as<double>());
        tolerance_percents.push_back(tolerances[i].as<double>());
    }
    
    // Create component variations
    std::vector<monte_carlo_sampler::component_variation> variations;
    for (size_t i = 0; i < nominal_values.size(); i++) {
        monte_carlo_sampler::component_variation var;
        var.nominal_value = nominal_values[i];
        var.tolerance_percent = tolerance_percents[i];
        var.distribution = monte_carlo_sampler::component_variation::GAUSSIAN;
        variations.push_back(var);
    }
    
    // Create network builder that works with C++ vectors
    auto cpp_network_builder = [&](const std::vector<double>& values) -> two_port {
        emscripten::val js_values = emscripten::val::array();
        for (size_t i = 0; i < values.size(); i++) {
            js_values.set(i, values[i]);
        }
        return network_builder(js_values);
    };
    
    // Generate Monte Carlo samples
    monte_carlo_sampler sampler;
    auto impedances = sampler.generate_impedance_samples(
        cpp_network_builder, variations, num_samples, frequency, complex(load_impedance, 0.0));
    
    // Convert to Smith chart points
    smith_chart_generator generator;
    auto points = generator.impedances_to_smith_points(impedances, z0);
    
    return emscripten::val(emscripten::typed_memory_view(points.size(), points.data()));
}

complex_wrapper impedance_to_reflection_wrapper(const complex_wrapper& impedance, double z0) {
    complex z = impedance.to_complex();
    complex gamma = smith_chart_generator::impedance_to_reflection(z, z0);
    return complex_wrapper(gamma);
}

complex_wrapper reflection_to_impedance_wrapper(const complex_wrapper& reflection, double z0) {
    complex gamma = reflection.to_complex();
    complex z = smith_chart_generator::reflection_to_impedance(gamma, z0);
    return complex_wrapper(z);
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
    
    // Stub functions
    function("seriesOpenStub", &create_series_open_stub);
    function("seriesShortStub", &create_series_short_stub);
    function("shuntOpenStub", &create_shunt_open_stub);
    function("shuntShortStub", &create_shunt_short_stub);
    function("quarterWaveSeriesOpenStub", &create_quarter_wave_series_open_stub);
    function("quarterWaveSeriesShortStub", &create_quarter_wave_series_short_stub);
    function("quarterWaveShuntOpenStub", &create_quarter_wave_shunt_open_stub);
    function("quarterWaveShuntShortStub", &create_quarter_wave_shunt_short_stub);
    
    // Conversion functions
    function("fromSParameters", &create_from_s_parameters);
    
    // Shunt tee functions
    function("shuntTee", &create_shunt_tee);
    function("shuntTeeShort", &create_shunt_tee_short);
    function("shuntTeeOpen", &create_shunt_tee_open);
    function("shuntTeeMatch", &create_shunt_tee_match);
    function("shuntStubFromTline", &create_shunt_stub_from_tline);
    function("shuntTeeShortStub", &create_shunt_tee_short_stub);
    function("shuntTeeOpenStub", &create_shunt_tee_open_stub);
    
    // Smith chart point generation
    function("generateNetworkSweepPoints", &generate_network_sweep_points);
    function("generateNetworkSweepPointsWithConfig", &generate_network_sweep_points_with_config);
    function("impedancesToSmithPoints", &impedances_to_smith_points);
    function("generateMonteCarloSamples", &generate_monte_carlo_samples);
    function("impedanceToReflection", &impedance_to_reflection_wrapper);
    function("reflectionToImpedance", &reflection_to_impedance_wrapper);
    
    // Operations
    function("cascade", &cascade_networks);
    
    // Utility functions
    function("butterworthLC3", &make_butterworth_lc_lowpass_3rd);
    function("piAttenuator", &make_pi_attenuator);
    function("tAttenuator", &make_t_attenuator);
    
    // Constants
    constant("PI", PI);
    constant("C0", C0);
    
    // Frequency sweep enums and structures
    enum_<sweep_type>("SweepType")
        .value("LINEAR", sweep_type::LINEAR)
        .value("LOG", sweep_type::LOG);
    
    class_<frequency_sweep>("FrequencySweep")
        .constructor<double, double, size_t, sweep_type>()
        .property("startFreq", &frequency_sweep::start_freq)
        .property("stopFreq", &frequency_sweep::stop_freq)
        .property("numPoints", &frequency_sweep::num_points)
        .property("type", &frequency_sweep::type)
        .function("getFrequencies", &frequency_sweep::get_frequencies);
    
    // Sweep results wrapper
    class_<sweep_results>("SweepResults")
        .property("frequencies", &sweep_results::frequencies)
        .property("sParams", &sweep_results::s_params)
        .property("inputImpedances", &sweep_results::input_impedances)
        .property("outputImpedances", &sweep_results::output_impedances)
        .function("getS11", &sweep_results::get_s11)
        .function("getS21", &sweep_results::get_s21)
        .function("getS11Db", &sweep_results::get_s11_db)
        .function("getS21Db", &sweep_results::get_s21_db)
        .function("getVswr", &sweep_results::get_vswr)
        .function("getS11PhaseDeg", &sweep_results::get_s11_phase_deg)
        .function("getS21PhaseDeg", &sweep_results::get_s21_phase_deg);
    
    // Register std::vector types for frequency sweep
    register_vector<double>("VectorDouble");
    register_vector<complex>("VectorComplex");
    register_vector<s_parameters>("VectorSParams");
}