#include <gtest/gtest.h>
#include "../include/cascadix.h"
#include <cmath>

using namespace cascadix;
using complex = std::complex<double>;

// Helper function to check complex numbers are approximately equal
bool complex_near(const complex& a, const complex& b, double tolerance = 1e-6) {
    return std::abs(a - b) < tolerance;
}

// Test basic series resistor
TEST(ComponentsTest, SeriesResistor) {
    double r = 50.0;
    two_port res = series_resistor(r);
    
    // Check ABCD matrix
    EXPECT_DOUBLE_EQ(res.a().real(), 1.0);
    EXPECT_DOUBLE_EQ(res.a().imag(), 0.0);
    EXPECT_DOUBLE_EQ(res.b().real(), r);
    EXPECT_DOUBLE_EQ(res.b().imag(), 0.0);
    EXPECT_DOUBLE_EQ(res.c().real(), 0.0);
    EXPECT_DOUBLE_EQ(res.c().imag(), 0.0);
    EXPECT_DOUBLE_EQ(res.d().real(), 1.0);
    EXPECT_DOUBLE_EQ(res.d().imag(), 0.0);
    
    // Check reciprocity
    EXPECT_TRUE(res.is_reciprocal());
}

// Test series inductor
TEST(ComponentsTest, SeriesInductor) {
    double l = 10e-9;  // 10 nH
    double freq = 1e9; // 1 GHz
    two_port ind_base = series_inductor(l, freq);
    // Create a reference to the derived class for testing specific methods
    series_inductor ind(l, freq);
    
    double omega = 2.0 * PI * freq;
    double expected_xl = omega * l;
    
    // Check impedance
    EXPECT_DOUBLE_EQ(ind.impedance().real(), 0.0);
    EXPECT_NEAR(ind.impedance().imag(), expected_xl, 1e-10);
    
    // Check ABCD matrix
    EXPECT_DOUBLE_EQ(ind.a().real(), 1.0);
    EXPECT_DOUBLE_EQ(ind.b().imag(), expected_xl);
    EXPECT_DOUBLE_EQ(ind.c().real(), 0.0);
    EXPECT_DOUBLE_EQ(ind.d().real(), 1.0);
    
    // Check reciprocity
    EXPECT_TRUE(ind.is_reciprocal());
}

// Test series capacitor
TEST(ComponentsTest, SeriesCapacitor) {
    double c = 1e-12;  // 1 pF
    double freq = 1e9; // 1 GHz
    two_port cap_base = series_capacitor(c, freq);
    // Create a reference to the derived class for testing specific methods
    series_capacitor cap(c, freq);
    
    double omega = 2.0 * PI * freq;
    double expected_xc = -1.0 / (omega * c);
    
    // Check impedance
    EXPECT_DOUBLE_EQ(cap.impedance().real(), 0.0);
    EXPECT_NEAR(cap.impedance().imag(), expected_xc, 1e-10);
    
    // Check ABCD matrix
    EXPECT_DOUBLE_EQ(cap.a().real(), 1.0);
    EXPECT_NEAR(cap.b().imag(), expected_xc, 1e-10);
    EXPECT_DOUBLE_EQ(cap.c().real(), 0.0);
    EXPECT_DOUBLE_EQ(cap.d().real(), 1.0);
}

// Test shunt resistor
TEST(ComponentsTest, ShuntResistor) {
    double r = 100.0;
    two_port res = shunt_resistor(r);
    
    // Check ABCD matrix
    EXPECT_DOUBLE_EQ(res.a().real(), 1.0);
    EXPECT_DOUBLE_EQ(res.b().real(), 0.0);
    EXPECT_DOUBLE_EQ(res.c().real(), 1.0 / r);
    EXPECT_DOUBLE_EQ(res.d().real(), 1.0);
    
    // Check reciprocity
    EXPECT_TRUE(res.is_reciprocal());
}

// Test shunt capacitor
TEST(ComponentsTest, ShuntCapacitor) {
    double c = 5e-12;  // 5 pF
    double freq = 2e9; // 2 GHz
    two_port cap_base = shunt_capacitor(c, freq);
    // Create a reference to the derived class for testing specific methods
    shunt_capacitor cap(c, freq);
    
    double omega = 2.0 * PI * freq;
    double expected_bc = omega * c;
    
    // Check admittance
    EXPECT_DOUBLE_EQ(cap.admittance().real(), 0.0);
    EXPECT_NEAR(cap.admittance().imag(), expected_bc, 1e-10);
    
    // Check ABCD matrix
    EXPECT_DOUBLE_EQ(cap.a().real(), 1.0);
    EXPECT_DOUBLE_EQ(cap.b().real(), 0.0);
    EXPECT_NEAR(cap.c().imag(), expected_bc, 1e-10);
    EXPECT_DOUBLE_EQ(cap.d().real(), 1.0);
}

// Test transmission line - quarter wave
TEST(ComponentsTest, QuarterWaveTransmissionLine) {
    double z0 = 50.0;
    double freq = 1e9;
    
    // Create quarter-wave line
    two_port tline = transmission_line::from_electrical_length(90.0, z0, freq);
    
    // For a quarter-wave line: A = D = 0, B = jZ0, C = j/Z0
    EXPECT_NEAR(std::abs(tline.a()), 0.0, 1e-10);
    EXPECT_NEAR(std::abs(tline.d()), 0.0, 1e-10);
    EXPECT_NEAR(std::abs(tline.b()), z0, 1e-6);
    EXPECT_NEAR(std::abs(tline.c()), 1.0 / z0, 1e-6);
    
    // Check that it's reciprocal
    EXPECT_TRUE(tline.is_reciprocal(1e-6));
}

// Test ideal transformer
TEST(ComponentsTest, IdealTransformer) {
    double n = 2.0;  // 2:1 turns ratio
    two_port xfmr_base = ideal_transformer(n);
    // Create a reference to the derived class for testing specific methods
    ideal_transformer xfmr(n);
    
    // Check ABCD matrix
    EXPECT_DOUBLE_EQ(xfmr.a().real(), n);
    EXPECT_DOUBLE_EQ(xfmr.b().real(), 0.0);
    EXPECT_DOUBLE_EQ(xfmr.c().real(), 0.0);
    EXPECT_DOUBLE_EQ(xfmr.d().real(), 1.0 / n);
    
    // Check reciprocity
    EXPECT_TRUE(xfmr.is_reciprocal());
    
    // Check impedance transformation
    EXPECT_DOUBLE_EQ(xfmr.impedance_ratio(), n * n);
}

// Test series RLC resonant circuit
TEST(ComponentsTest, SeriesRLC) {
    double r = 10.0;
    double l = 100e-9;  // 100 nH
    double c = 0.2533e-12;  // 0.2533 pF for 1 GHz resonance
    double freq = 1e9;
    
    two_port rlc_base = series_rlc(r, l, c, freq);
    series_rlc rlc(r, l, c, freq);
    
    // Check resonant frequency (should be close to 1 GHz)
    double f_res = rlc.resonant_frequency();
    EXPECT_NEAR(f_res, 1e9, 1e6);  // Within 1 MHz
    
    // At resonance, impedance should be purely resistive
    series_rlc rlc_at_res(r, l, c, f_res);
    complex z = rlc_at_res.impedance();
    EXPECT_NEAR(z.real(), r, 1e-6);
    EXPECT_NEAR(std::abs(z.imag()), 0.0, 1e-6);
    
    // Check Q factor
    double q = rlc.q_factor();
    EXPECT_GT(q, 0.0);
}

// Test shunt RLC resonant circuit
TEST(ComponentsTest, ShuntRLC) {
    double r = 1000.0;
    double l = 100e-9;  // 100 nH
    double c = 0.2533e-12;  // 0.2533 pF for 1 GHz resonance
    double freq = 1e9;
    
    two_port rlc_base = shunt_rlc(r, l, c, freq);
    shunt_rlc rlc(r, l, c, freq);
    
    // Check resonant frequency
    double f_res = rlc.resonant_frequency();
    EXPECT_NEAR(f_res, 1e9, 1e6);  // Within 1 MHz
    
    // At resonance, admittance should be purely conductive
    shunt_rlc rlc_at_res(r, l, c, f_res);
    complex y = rlc_at_res.admittance();
    EXPECT_NEAR(y.real(), 1.0 / r, 1e-6);
    EXPECT_NEAR(std::abs(y.imag()), 0.0, 1e-6);
}

// Test factory functions
TEST(ComponentsTest, FactoryFunctions) {
    double freq = 2.4e9;
    
    auto r = make_series_r(50.0);
    EXPECT_DOUBLE_EQ(r.b().real(), 50.0);
    
    auto l = make_series_l(10e-9, freq);
    EXPECT_GT(l.b().imag(), 0.0);  // Inductive reactance is positive
    
    auto c = make_series_c(1e-12, freq);
    EXPECT_LT(c.b().imag(), 0.0);  // Capacitive reactance is negative
    
    auto tline = make_quarter_wave_tline(75.0, freq);
    EXPECT_NEAR(std::abs(tline.b()), 75.0, 1.0);
}