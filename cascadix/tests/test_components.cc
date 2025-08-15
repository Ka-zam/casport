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

// Test series open stub
TEST(ComponentsTest, SeriesOpenStub) {
    double freq = 1e9;  // 1 GHz
    double z0 = 50.0;   // 50Ω
    double length = 0.075;  // λ/4 at 1GHz (wavelength ≈ 0.3m)
    
    series_open_stub stub(length, z0, freq);
    
    // Quarter-wave open stub should present short circuit (Z ≈ 0)
    complex z_in = stub.input_impedance();
    EXPECT_NEAR(z_in.real(), 0.0, 1.0);  // Very small resistance
    EXPECT_LT(std::abs(z_in.imag()), 10.0);  // Small reactance
    
    // Check ABCD matrix elements
    EXPECT_DOUBLE_EQ(stub.a().real(), 1.0);
    EXPECT_DOUBLE_EQ(stub.a().imag(), 0.0);
    EXPECT_DOUBLE_EQ(stub.c().real(), 0.0);
    EXPECT_DOUBLE_EQ(stub.c().imag(), 0.0);
    EXPECT_DOUBLE_EQ(stub.d().real(), 1.0);
    EXPECT_DOUBLE_EQ(stub.d().imag(), 0.0);
    
    // B element should equal input impedance
    EXPECT_TRUE(complex_near(stub.b(), z_in, 1e-10));
    
    // Should be reciprocal
    EXPECT_TRUE(stub.is_reciprocal());
}

// Test series short stub
TEST(ComponentsTest, SeriesShortStub) {
    double freq = 1e9;
    double z0 = 75.0;   // 75Ω characteristic impedance
    double length = 0.075;  // λ/4 at 1GHz
    
    series_short_stub stub(length, z0, freq);
    
    // Quarter-wave short stub should present open circuit (Z → ∞)
    complex z_in = stub.input_impedance();
    EXPECT_GT(std::abs(z_in), 1000.0);  // Very high impedance
    
    // Check ABCD matrix structure (series impedance)
    EXPECT_DOUBLE_EQ(stub.a().real(), 1.0);
    EXPECT_DOUBLE_EQ(stub.c().real(), 0.0);
    EXPECT_DOUBLE_EQ(stub.d().real(), 1.0);
    
    // Should be reciprocal
    EXPECT_TRUE(stub.is_reciprocal());
}

// Test shunt open stub
TEST(ComponentsTest, ShuntOpenStub) {
    double freq = 2e9;  // 2 GHz
    double z0 = 50.0;
    double length = 0.0375;  // λ/4 at 2GHz
    
    shunt_open_stub stub(length, z0, freq);
    
    // Quarter-wave open stub in shunt should present short circuit to ground
    complex y_in = stub.input_admittance();
    EXPECT_GT(std::abs(y_in), 0.01);  // Should have significant admittance
    
    // Check ABCD matrix structure (shunt admittance)
    EXPECT_DOUBLE_EQ(stub.a().real(), 1.0);
    EXPECT_DOUBLE_EQ(stub.a().imag(), 0.0);
    EXPECT_DOUBLE_EQ(stub.b().real(), 0.0);
    EXPECT_DOUBLE_EQ(stub.b().imag(), 0.0);
    EXPECT_DOUBLE_EQ(stub.d().real(), 1.0);
    EXPECT_DOUBLE_EQ(stub.d().imag(), 0.0);
    
    // C element should equal input admittance
    EXPECT_TRUE(complex_near(stub.c(), y_in, 1e-10));
    
    // Should be reciprocal
    EXPECT_TRUE(stub.is_reciprocal());
}

// Test shunt short stub
TEST(ComponentsTest, ShuntShortStub) {
    double freq = 2e9;
    double z0 = 50.0;
    double length = 0.0375;  // λ/4 at 2GHz
    
    shunt_short_stub stub(length, z0, freq);
    
    // Quarter-wave short stub in shunt should present open circuit to ground
    complex y_in = stub.input_admittance();
    EXPECT_LT(std::abs(y_in), 0.001);  // Very small admittance (high impedance)
    
    // Check ABCD matrix structure (shunt admittance)
    EXPECT_DOUBLE_EQ(stub.a().real(), 1.0);
    EXPECT_DOUBLE_EQ(stub.b().real(), 0.0);
    EXPECT_DOUBLE_EQ(stub.d().real(), 1.0);
    
    // Should be reciprocal
    EXPECT_TRUE(stub.is_reciprocal());
}

// Test stub impedance transformations
TEST(ComponentsTest, StubImpedanceTransformation) {
    double freq = 1e9;
    double z0 = 50.0;
    
    // Test that quarter-wave stubs transform correctly
    double qw_length = 0.075;  // λ/4
    
    // Open stub at λ/4 should present very low impedance (approaching short)
    series_open_stub open_stub(qw_length, z0, freq);
    complex z_open = open_stub.input_impedance();
    EXPECT_LT(std::abs(z_open), 10.0);  // Should be close to short circuit
    
    // Short stub at λ/4 should present very high impedance (approaching open)
    series_short_stub short_stub(qw_length, z0, freq);
    complex z_short = short_stub.input_impedance();
    EXPECT_GT(std::abs(z_short), 1000.0);  // Should be close to open circuit
    
    // For shunt stubs, check admittance behavior
    shunt_open_stub shunt_open(qw_length, z0, freq);
    complex y_shunt_open = shunt_open.input_admittance();
    EXPECT_GT(std::abs(y_shunt_open), 0.01);  // Should have high admittance (low impedance to ground)
    
    shunt_short_stub shunt_short(qw_length, z0, freq);
    complex y_shunt_short = shunt_short.input_admittance();
    EXPECT_LT(std::abs(y_shunt_short), 0.001);  // Should have very low admittance (high impedance to ground)
}

// Test factory functions for stubs
TEST(ComponentsTest, StubFactoryFunctions) {
    double freq = 1e9;
    double z0 = 50.0;
    double length = 0.01;  // 1cm
    
    // Test all factory functions create valid two_port objects
    auto series_open = make_series_open_stub(length, z0, freq);
    auto series_short = make_series_short_stub(length, z0, freq);
    auto shunt_open = make_shunt_open_stub(length, z0, freq);
    auto shunt_short = make_shunt_short_stub(length, z0, freq);
    
    // All should be reciprocal
    EXPECT_TRUE(series_open.is_reciprocal());
    EXPECT_TRUE(series_short.is_reciprocal());
    EXPECT_TRUE(shunt_open.is_reciprocal());
    EXPECT_TRUE(shunt_short.is_reciprocal());
    
    // Test quarter-wave factory functions
    auto qw_series_open = make_quarter_wave_series_open_stub(z0, freq);
    auto qw_series_short = make_quarter_wave_series_short_stub(z0, freq);
    auto qw_shunt_open = make_quarter_wave_shunt_open_stub(z0, freq);
    auto qw_shunt_short = make_quarter_wave_shunt_short_stub(z0, freq);
    
    // All should be reciprocal
    EXPECT_TRUE(qw_series_open.is_reciprocal());
    EXPECT_TRUE(qw_series_short.is_reciprocal());
    EXPECT_TRUE(qw_shunt_open.is_reciprocal());
    EXPECT_TRUE(qw_shunt_short.is_reciprocal());
}

// Test shunt_tee component
TEST(ComponentsTest, ShuntTee) {
    double freq = 1e9;
    double c = 1e-12;  // 1 pF
    
    // Create a series capacitor
    series_capacitor cap(c, freq);
    
    // Create shunt_tee with short termination (should act like shunt capacitor)
    shunt_tee tee_cap = shunt_tee::short_terminated(cap);
    
    // Create reference shunt capacitor
    shunt_capacitor shunt_cap(c, freq);
    
    // ABCD matrices should be identical (within tolerance)
    EXPECT_TRUE(complex_near(tee_cap.a(), shunt_cap.a(), 1e-10));
    EXPECT_TRUE(complex_near(tee_cap.b(), shunt_cap.b(), 1e-10));
    EXPECT_TRUE(complex_near(tee_cap.c(), shunt_cap.c(), 1e-10));
    EXPECT_TRUE(complex_near(tee_cap.d(), shunt_cap.d(), 1e-10));
    
    // Both should be reciprocal
    EXPECT_TRUE(tee_cap.is_reciprocal());
    EXPECT_TRUE(shunt_cap.is_reciprocal());
}

// Test shunt_tee stub equivalence
TEST(ComponentsTest, ShuntTeeStubEquivalence) {
    double freq = 2e9;
    double z0 = 50.0;
    double length = 0.01;  // 1cm
    
    // Create stub using existing dedicated component
    shunt_short_stub dedicated_stub(length, z0, freq);
    
    // Create equivalent stub using shunt_tee
    transmission_line tline(length, z0, freq);
    shunt_tee tee_stub = shunt_tee::short_terminated(tline);
    
    // ABCD matrices should be equivalent
    EXPECT_TRUE(complex_near(dedicated_stub.a(), tee_stub.a(), 1e-10));
    EXPECT_TRUE(complex_near(dedicated_stub.b(), tee_stub.b(), 1e-10));
    EXPECT_TRUE(complex_near(dedicated_stub.c(), tee_stub.c(), 1e-10));
    EXPECT_TRUE(complex_near(dedicated_stub.d(), tee_stub.d(), 1e-10));
    
    // Test with factory function as well
    auto factory_stub = make_shunt_tee_short_stub(length, z0, freq);
    EXPECT_TRUE(complex_near(dedicated_stub.a(), factory_stub.a(), 1e-10));
    EXPECT_TRUE(complex_near(dedicated_stub.b(), factory_stub.b(), 1e-10));
    EXPECT_TRUE(complex_near(dedicated_stub.c(), factory_stub.c(), 1e-10));
    EXPECT_TRUE(complex_near(dedicated_stub.d(), factory_stub.d(), 1e-10));
}

// Test shunt_tee with complex networks
TEST(ComponentsTest, ShuntTeeComplexNetwork) {
    double freq = 1e9;
    double z0 = 50.0;
    
    // Create a complex shunt network: transmission line + series LC
    auto tline = transmission_line(0.01, z0, freq);
    auto inductor = series_inductor(5e-9, freq);  // 5nH
    auto capacitor = series_capacitor(2e-12, freq);  // 2pF
    
    auto complex_network = tline * inductor * capacitor;
    
    // Create shunt_tee with different terminations
    auto short_terminated = shunt_tee::short_terminated(complex_network);
    auto open_terminated = shunt_tee::open_terminated(complex_network);
    auto match_terminated = shunt_tee::match_terminated(complex_network, z0);
    
    // All should be reciprocal
    EXPECT_TRUE(short_terminated.is_reciprocal());
    EXPECT_TRUE(open_terminated.is_reciprocal());
    EXPECT_TRUE(match_terminated.is_reciprocal());
    
    // Terminations should produce different results
    EXPECT_FALSE(complex_near(short_terminated.c(), open_terminated.c(), 1e-6));
    EXPECT_FALSE(complex_near(short_terminated.c(), match_terminated.c(), 1e-6));
    
    // Check that shunt impedances are sensible
    complex z_short = short_terminated.shunt_impedance();
    complex z_open = open_terminated.shunt_impedance();
    complex z_match = match_terminated.shunt_impedance();
    
    EXPECT_TRUE(std::isfinite(z_short.real()));
    EXPECT_TRUE(std::isfinite(z_short.imag()));
    EXPECT_TRUE(std::isfinite(z_open.real()));
    EXPECT_TRUE(std::isfinite(z_open.imag()));
    EXPECT_TRUE(std::isfinite(z_match.real()));
    EXPECT_TRUE(std::isfinite(z_match.imag()));
}

// Test shunt_tee factory functions
TEST(ComponentsTest, ShuntTeeFactoryFunctions) {
    double freq = 1e9;
    
    // Create a series inductor
    auto inductor = series_inductor(10e-9, freq);
    
    // Test various factory functions
    auto tee1 = make_shunt_tee(inductor);  // Default short termination
    auto tee2 = make_shunt_tee_short(inductor);
    auto tee3 = make_shunt_tee_open(inductor);
    auto tee4 = make_shunt_tee_match(inductor, 50.0);
    
    // Short-terminated versions should be identical
    EXPECT_TRUE(complex_near(tee1.a(), tee2.a(), 1e-12));
    EXPECT_TRUE(complex_near(tee1.b(), tee2.b(), 1e-12));
    EXPECT_TRUE(complex_near(tee1.c(), tee2.c(), 1e-12));
    EXPECT_TRUE(complex_near(tee1.d(), tee2.d(), 1e-12));
    
    // All should be reciprocal
    EXPECT_TRUE(tee1.is_reciprocal());
    EXPECT_TRUE(tee2.is_reciprocal());
    EXPECT_TRUE(tee3.is_reciprocal());
    EXPECT_TRUE(tee4.is_reciprocal());
    
    // Different terminations should give different results
    EXPECT_FALSE(complex_near(tee2.c(), tee3.c(), 1e-6));
    EXPECT_FALSE(complex_near(tee2.c(), tee4.c(), 1e-6));
}