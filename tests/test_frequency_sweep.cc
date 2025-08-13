#include <gtest/gtest.h>
#include "../include/cascadix.h"
#include <cmath>

using namespace cascadix;
using complex = std::complex<double>;

// Test linear frequency sweep generation
TEST(FrequencySweepTest, LinearSweep) {
    frequency_sweep sweep(1e9, 10e9, 10, sweep_type::LINEAR);
    auto frequencies = sweep.get_frequencies();
    
    EXPECT_EQ(frequencies.size(), 10);
    EXPECT_DOUBLE_EQ(frequencies[0], 1e9);
    EXPECT_DOUBLE_EQ(frequencies[9], 10e9);
    
    // Check linear spacing
    double step = 1e9; // (10e9 - 1e9) / 9
    for (size_t i = 1; i < frequencies.size(); ++i) {
        EXPECT_NEAR(frequencies[i] - frequencies[i-1], step, 1e3);
    }
}

// Test logarithmic frequency sweep generation
TEST(FrequencySweepTest, LogSweep) {
    frequency_sweep sweep(1e6, 1e9, 4, sweep_type::LOG);
    auto frequencies = sweep.get_frequencies();
    
    EXPECT_EQ(frequencies.size(), 4);
    EXPECT_DOUBLE_EQ(frequencies[0], 1e6);
    EXPECT_DOUBLE_EQ(frequencies[3], 1e9);
    
    // Check logarithmic spacing (each step should be 10x)
    EXPECT_NEAR(frequencies[1], 1e7, 1e3);
    EXPECT_NEAR(frequencies[2], 1e8, 1e5);
}

// Test S-parameter sweep of a simple resistor
TEST(FrequencySweepTest, ResistorSweep) {
    // A resistor should have frequency-independent S-parameters
    auto builder = [](double freq) {
        return series_resistor(25.0);
    };
    
    frequency_sweep sweep(1e9, 10e9, 5);
    auto s_params = sweep_s_parameters(builder, sweep, 50.0);
    
    EXPECT_EQ(s_params.size(), 5);
    
    // All S-parameters should be the same for a resistor
    for (const auto& s : s_params) {
        EXPECT_NEAR(std::abs(s.s11), std::abs(s_params[0].s11), 1e-10);
        EXPECT_NEAR(std::abs(s.s21), std::abs(s_params[0].s21), 1e-10);
    }
}

// Test frequency sweep of a series LC circuit
TEST(FrequencySweepTest, SeriesLCSweep) {
    double l = 10e-9;  // 10 nH
    double c = 1e-12;  // 1 pF
    
    auto builder = [l, c](double freq) {
        two_port inductor = series_inductor(l, freq);
        two_port capacitor = series_capacitor(c, freq);
        return inductor * capacitor;
    };
    
    // Sweep around resonance (about 1.59 GHz)
    frequency_sweep sweep(1e9, 3e9, 100);
    auto results = perform_sweep(builder, sweep);
    
    EXPECT_EQ(results.frequencies.size(), 100);
    EXPECT_EQ(results.s_params.size(), 100);
    
    // Find minimum S11 (should be near resonance)
    auto s11_db = results.get_s11_db();
    auto min_iter = std::min_element(s11_db.begin(), s11_db.end());
    size_t min_index = std::distance(s11_db.begin(), min_iter);
    
    double f_resonance = 1.0 / (2.0 * PI * std::sqrt(l * c));
    EXPECT_NEAR(results.frequencies[min_index], f_resonance, f_resonance * 0.1);
}

// Test Butterworth filter sweep
TEST(FrequencySweepTest, ButterworthFilterSweep) {
    double fc = 1e9;  // 1 GHz cutoff
    double z0 = 50.0;
    
    auto builder = make_butterworth_builder(fc, z0);
    
    // Linear sweep from 0.1 to 10 GHz with more points around cutoff
    frequency_sweep sweep(0.1e9, 2e9, 100);
    auto results = perform_sweep(builder, sweep);
    
    auto s21_db = results.get_s21_db();
    
    // Find the frequency closest to cutoff
    size_t cutoff_index = 0;
    double min_diff = std::abs(results.frequencies[0] - fc);
    for (size_t i = 1; i < results.frequencies.size(); ++i) {
        double diff = std::abs(results.frequencies[i] - fc);
        if (diff < min_diff) {
            min_diff = diff;
            cutoff_index = i;
        }
    }
    
    // At cutoff, a 3rd order Butterworth should be approximately -3dB
    EXPECT_NEAR(s21_db[cutoff_index], -3.0, 1.5);
    
    // Check rolloff above cutoff (at 2 GHz, should have significant attenuation)
    EXPECT_LT(s21_db.back(), -10.0);  // Should have significant attenuation at 2 GHz
}

// Test transmission line sweep
TEST(FrequencySweepTest, TransmissionLineSweep) {
    double length = 0.075;  // Quarter wavelength at 1 GHz
    double z0 = 50.0;
    
    auto builder = make_tline_builder(length, z0);
    
    // Sweep from 0.5 to 2 GHz
    frequency_sweep sweep(0.5e9, 2e9, 30);
    auto results = perform_sweep(builder, sweep, z0, complex(100.0, 0.0));
    
    // At 1 GHz, should transform 100Ω to 25Ω
    size_t index_1ghz = 10;  // Approximate
    complex z_in = results.input_impedances[index_1ghz];
    EXPECT_NEAR(z_in.real(), 25.0, 5.0);
    EXPECT_NEAR(z_in.imag(), 0.0, 5.0);
}

// Test cascade of multiple builders
TEST(FrequencySweepTest, CascadeBuilders) {
    // Create a pi attenuator from individual components
    auto r1_builder = [](double freq) { return shunt_resistor(100.0); };
    auto r2_builder = [](double freq) { return series_resistor(50.0); };
    auto r3_builder = [](double freq) { return shunt_resistor(100.0); };
    
    auto cascaded = cascade_builders({r1_builder, r2_builder, r3_builder});
    
    frequency_sweep sweep(1e9, 2e9, 10);
    auto s_params = sweep_s_parameters(cascaded, sweep);
    
    // Should have constant attenuation across frequency
    for (const auto& s : s_params) {
        EXPECT_NEAR(std::abs(s.s21), std::abs(s_params[0].s21), 1e-10);
    }
}

// Test VSWR calculation across frequency
TEST(FrequencySweepTest, VSWRSweep) {
    // Create a simple L-match network
    auto builder = make_l_match_builder(50.0, 100.0, false);
    
    // The match is designed for a specific frequency, so VSWR should vary
    frequency_sweep sweep(1e9, 3e9, 20);
    auto results = perform_sweep(builder, sweep);
    
    auto vswr_values = results.get_vswr();
    
    EXPECT_EQ(vswr_values.size(), 20);
    
    // VSWR should be finite and positive
    for (double vswr : vswr_values) {
        EXPECT_GT(vswr, 1.0);
        EXPECT_LT(vswr, 100.0);  // Reasonable upper bound
    }
}

// Test phase extraction
TEST(FrequencySweepTest, PhaseExtraction) {
    // Transmission line has linear phase vs frequency
    double length = 0.01;  // 1 cm
    double z0 = 50.0;
    
    auto builder = make_tline_builder(length, z0);
    
    frequency_sweep sweep(1e9, 10e9, 10, sweep_type::LINEAR);
    auto results = perform_sweep(builder, sweep);
    
    auto phases = results.get_s21_phase_deg();
    
    EXPECT_EQ(phases.size(), 10);
    
    // Phase should decrease linearly with frequency (more negative)
    for (size_t i = 1; i < phases.size(); ++i) {
        // Account for phase wrapping
        double phase_diff = phases[i] - phases[i-1];
        if (phase_diff > 180) phase_diff -= 360;
        if (phase_diff < -180) phase_diff += 360;
        EXPECT_LT(phase_diff, 0);  // Phase should be decreasing
    }
}