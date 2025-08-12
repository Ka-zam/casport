#include <gtest/gtest.h>
#include "../include/cascadix.h"
#include <cmath>

using namespace cascadix;
using complex = std::complex<double>;

// Helper function
bool complex_near(const complex& a, const complex& b, double tolerance = 1e-6) {
    return std::abs(a - b) < tolerance;
}

// Test identity cascading
TEST(CascadingTest, IdentityCascade) {
    two_port id1 = identity_two_port();
    two_port id2 = identity_two_port();
    
    two_port result = id1 * id2;
    
    // Identity × Identity = Identity
    EXPECT_TRUE(complex_near(result.a(), 1.0));
    EXPECT_TRUE(complex_near(result.b(), 0.0));
    EXPECT_TRUE(complex_near(result.c(), 0.0));
    EXPECT_TRUE(complex_near(result.d(), 1.0));
}

// Test cascading series elements
TEST(CascadingTest, SeriesElementsCascade) {
    two_port r1 = series_resistor(25.0);
    two_port r2 = series_resistor(25.0);
    
    two_port result = r1 * r2;
    
    // Two series resistors should give total resistance
    EXPECT_DOUBLE_EQ(result.a().real(), 1.0);
    EXPECT_DOUBLE_EQ(result.b().real(), 50.0);  // 25 + 25
    EXPECT_DOUBLE_EQ(result.c().real(), 0.0);
    EXPECT_DOUBLE_EQ(result.d().real(), 1.0);
}

// Test cascading shunt elements
TEST(CascadingTest, ShuntElementsCascade) {
    two_port r1 = shunt_resistor(100.0);
    two_port r2 = shunt_resistor(100.0);
    
    two_port result = r1 * r2;
    
    // Two shunt resistors in cascade
    EXPECT_DOUBLE_EQ(result.a().real(), 1.0);
    EXPECT_DOUBLE_EQ(result.b().real(), 0.0);
    EXPECT_DOUBLE_EQ(result.c().real(), 0.02);  // 1/100 + 1/100
    EXPECT_DOUBLE_EQ(result.d().real(), 1.0);
}

// Test L-network (series then shunt)
TEST(CascadingTest, LNetwork) {
    double freq = 1e9;
    two_port l = series_inductor(10e-9, freq);
    two_port c = shunt_capacitor(1e-12, freq);
    
    two_port l_network = l * c;
    
    // Check that cascading works correctly
    EXPECT_TRUE(l_network.is_reciprocal());
    
    // Calculate expected values
    double omega = 2.0 * PI * freq;
    complex z_l(0.0, omega * 10e-9);
    complex y_c(0.0, omega * 1e-12);
    
    // Expected ABCD for L-network: A = 1 + ZY, B = Z, C = Y, D = 1
    complex expected_a = 1.0 + z_l * y_c;
    
    EXPECT_TRUE(complex_near(l_network.a(), expected_a));
    EXPECT_TRUE(complex_near(l_network.b(), z_l));
    EXPECT_TRUE(complex_near(l_network.c(), y_c));
    EXPECT_TRUE(complex_near(l_network.d(), 1.0));
}

// Test T-network
TEST(CascadingTest, TNetwork) {
    two_port r1 = series_resistor(25.0);
    two_port r2 = shunt_resistor(100.0);
    two_port r3 = series_resistor(25.0);
    
    two_port t_network = r1 * r2 * r3;
    
    // Check reciprocity
    EXPECT_TRUE(t_network.is_reciprocal());
    
    // For T-network: A = 1 + Z1/Z2, B = Z1 + Z3 + Z1*Z3/Z2
    // C = 1/Z2, D = 1 + Z3/Z2
    double z1 = 25.0, z2 = 100.0, z3 = 25.0;
    double expected_a = 1.0 + z1 / z2;
    double expected_b = z1 + z3 + (z1 * z3) / z2;
    double expected_c = 1.0 / z2;
    double expected_d = 1.0 + z3 / z2;
    
    EXPECT_NEAR(t_network.a().real(), expected_a, 1e-10);
    EXPECT_NEAR(t_network.b().real(), expected_b, 1e-10);
    EXPECT_NEAR(t_network.c().real(), expected_c, 1e-10);
    EXPECT_NEAR(t_network.d().real(), expected_d, 1e-10);
}

// Test Pi-network
TEST(CascadingTest, PiNetwork) {
    two_port r1 = shunt_resistor(100.0);
    two_port r2 = series_resistor(50.0);
    two_port r3 = shunt_resistor(100.0);
    
    two_port pi_network = r1 * r2 * r3;
    
    // Check reciprocity
    EXPECT_TRUE(pi_network.is_reciprocal());
    
    // For Pi-network: A = 1 + Z/Y3, B = Z
    // C = Y1 + Y3 + Y1*Y3*Z, D = 1 + Z*Y1
    double y1 = 0.01, z = 50.0, y3 = 0.01;
    double expected_a = 1.0 + z * y3;
    double expected_b = z;
    double expected_c = y1 + y3 + y1 * y3 * z;
    double expected_d = 1.0 + z * y1;
    
    EXPECT_NEAR(pi_network.a().real(), expected_a, 1e-10);
    EXPECT_NEAR(pi_network.b().real(), expected_b, 1e-10);
    EXPECT_NEAR(pi_network.c().real(), expected_c, 1e-10);
    EXPECT_NEAR(pi_network.d().real(), expected_d, 1e-10);
}

// Test cascading with transmission lines
TEST(CascadingTest, TransmissionLineCascade) {
    double freq = 1e9;
    double z0 = 50.0;
    
    // Two eighth-wave lines should give a quarter-wave line
    two_port tline1 = transmission_line::from_electrical_length(45.0, z0, freq);
    two_port tline2 = transmission_line::from_electrical_length(45.0, z0, freq);
    
    two_port cascaded = tline1 * tline2;
    
    // Compare with a single quarter-wave line
    two_port quarter_wave = transmission_line::from_electrical_length(90.0, z0, freq);
    
    EXPECT_TRUE(complex_near(cascaded.a(), quarter_wave.a(), 1e-6));
    EXPECT_TRUE(complex_near(cascaded.b(), quarter_wave.b(), 1e-6));
    EXPECT_TRUE(complex_near(cascaded.c(), quarter_wave.c(), 1e-6));
    EXPECT_TRUE(complex_near(cascaded.d(), quarter_wave.d(), 1e-6));
}

// Test cascading with transformers
TEST(CascadingTest, TransformerCascade) {
    two_port t1 = ideal_transformer(2.0);   // 2:1
    two_port t2 = ideal_transformer(0.5);   // 1:2
    
    two_port cascaded = t1 * t2;
    
    // 2:1 followed by 1:2 should give identity
    EXPECT_NEAR(cascaded.a().real(), 1.0, 1e-10);
    EXPECT_NEAR(cascaded.b().real(), 0.0, 1e-10);
    EXPECT_NEAR(cascaded.c().real(), 0.0, 1e-10);
    EXPECT_NEAR(cascaded.d().real(), 1.0, 1e-10);
}

// Test complex filter cascade
TEST(CascadingTest, ButterworthFilter) {
    double fc = 1e9;
    double z0 = 50.0;
    
    // Create 3rd order Butterworth lowpass
    auto filter = make_butterworth_lc_lowpass_3rd(fc, z0);
    
    // Check reciprocity
    EXPECT_TRUE(filter.is_reciprocal(1e-6));
    
    // At DC, should have unity gain
    two_port l1 = series_inductor(6.1e-9, 1e3);  // Very low frequency
    two_port c2 = shunt_capacitor(58.8e-12, 1e3);
    two_port l3 = series_inductor(6.1e-9, 1e3);
    two_port filter_dc = l1 * c2 * l3;
    
    // Input impedance with matched load should be close to z0 at low freq
    complex z_in = filter_dc.input_impedance(z0);
    EXPECT_NEAR(z_in.real(), z0, 5.0);  // Some deviation expected
}

// Test operator *= for in-place cascading
TEST(CascadingTest, InPlaceCascade) {
    two_port network(1.0, 0.0, 0.0, 1.0);  // Identity
    two_port r1 = series_resistor(25.0);
    two_port r2 = series_resistor(25.0);
    
    network *= r1;
    network *= r2;
    
    // Should have total resistance of 50 ohms
    EXPECT_DOUBLE_EQ(network.b().real(), 50.0);
}

// Test cascading many elements
TEST(CascadingTest, LongCascade) {
    double freq = 2.4e9;
    
    // Build a chain of elements
    two_port chain = identity_two_port();
    
    for (int i = 0; i < 10; i++) {
        two_port r = series_resistor(1.0);
        chain *= r;
    }
    
    // Should have total series resistance of 10 ohms
    EXPECT_NEAR(chain.b().real(), 10.0, 1e-10);
    EXPECT_TRUE(chain.is_reciprocal());
}