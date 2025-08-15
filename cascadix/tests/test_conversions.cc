#include <gtest/gtest.h>
#include "../include/cascadix.h"
#include <cmath>

using namespace cascadix;
using complex = std::complex<double>;

// Helper function
bool complex_near(const complex& a, const complex& b, double tolerance = 1e-6) {
    return std::abs(a - b) < tolerance;
}

// Test S-parameter conversion for series resistor
TEST(ConversionsTest, SeriesResistorToSParams) {
    double r = 50.0;
    double z0 = 50.0;
    two_port res = series_resistor(r);
    
    auto s = res.to_s_parameters(z0);
    
    // For 50Ω series resistor with 50Ω reference:
    // Total impedance = 100Ω, reflection = (100-50)/(100+50) = 1/3
    EXPECT_NEAR(s.s11.real(), 1.0/3.0, 1e-6);
    EXPECT_NEAR(s.s11.imag(), 0.0, 1e-6);
    
    // Transmission = 2Z0/(Z_total + Z0) = 100/150 = 2/3
    EXPECT_NEAR(s.s21.real(), 2.0/3.0, 1e-6);
    EXPECT_NEAR(s.s21.imag(), 0.0, 1e-6);
    
    // For reciprocal network, S12 = S21
    EXPECT_TRUE(complex_near(s.s12, s.s21));
    
    // S22 = S11 for symmetric network
    EXPECT_TRUE(complex_near(s.s22, s.s11));
}

// Test S-parameter conversion for shunt resistor
TEST(ConversionsTest, ShuntResistorToSParams) {
    double r = 100.0;
    double z0 = 50.0;
    two_port res = shunt_resistor(r);
    
    auto s = res.to_s_parameters(z0);
    
    // For shunt 100Ω with 50Ω reference:
    // Parallel combination = 50||100 = 33.33Ω
    // S11 = (33.33 - 50)/(33.33 + 50) = -1/5
    EXPECT_NEAR(s.s11.real(), -1.0/5.0, 1e-6);
    EXPECT_NEAR(s.s11.imag(), 0.0, 1e-6);
    
    // S21 = 2 * 33.33 / 83.33 = 4/5
    EXPECT_NEAR(s.s21.real(), 4.0/5.0, 1e-6);
    EXPECT_NEAR(s.s21.imag(), 0.0, 1e-6);
}

// Test Z-parameter conversion
TEST(ConversionsTest, ToZParameters) {
    double freq = 1e9;
    two_port l = series_inductor(10e-9, freq);
    two_port c = shunt_capacitor(1e-12, freq);
    
    two_port lc = l * c;
    auto z_params = lc.to_z_parameters();
    
    // For L-C network, check reciprocity
    EXPECT_TRUE(complex_near(z_params.z12, z_params.z21));
}

// Test Y-parameter conversion
TEST(ConversionsTest, ToYParameters) {
    two_port r1 = shunt_resistor(100.0);
    two_port r2 = series_resistor(50.0);
    two_port r3 = shunt_resistor(100.0);
    
    two_port pi = r1 * r2 * r3;
    auto y_params = pi.to_y_parameters();
    
    // Check reciprocity
    EXPECT_TRUE(complex_near(y_params.y12, y_params.y21));
    
    // Y11 should include contribution from first shunt
    EXPECT_GT(y_params.y11.real(), 0.01);  // > 1/100
}

// Test return loss and VSWR
TEST(ConversionsTest, ReturnLossAndVSWR) {
    // Perfect match - series 0Ω
    two_port r0 = series_resistor(0.0);
    auto s_match = r0.to_s_parameters(50.0);
    
    EXPECT_NEAR(std::abs(s_match.s11), 0.0, 1e-6);
    EXPECT_GT(s_match.return_loss_db(), 60.0);  // Very high return loss
    EXPECT_NEAR(s_match.vswr(), 1.0, 1e-6);  // VSWR = 1 for perfect match
    
    // Total mismatch - series 50Ω (total 100Ω)
    two_port r50 = series_resistor(50.0);
    auto s_mismatch = r50.to_s_parameters(50.0);
    
    EXPECT_NEAR(std::abs(s_mismatch.s11), 1.0/3.0, 1e-6);
    double expected_rl = -20.0 * std::log10(1.0/3.0);
    EXPECT_NEAR(s_mismatch.return_loss_db(), expected_rl, 0.1);
    EXPECT_NEAR(s_mismatch.vswr(), 2.0, 1e-6);  // VSWR = 2 for 2:1 mismatch
}

// Test insertion loss
TEST(ConversionsTest, InsertionLoss) {
    // 3dB attenuator
    auto atten = make_pi_attenuator(3.0, 50.0);
    auto s = atten.to_s_parameters(50.0);
    
    // Should have approximately 3dB insertion loss
    EXPECT_NEAR(s.insertion_loss_db(), 3.0, 0.5);
    
    // Should be well matched
    EXPECT_LT(std::abs(s.s11), 0.1);
    EXPECT_LT(std::abs(s.s22), 0.1);
}

// Test input impedance calculation
TEST(ConversionsTest, InputImpedance) {
    double freq = 1e9;
    double z0 = 50.0;
    
    // Quarter-wave transformer
    two_port qwt = transmission_line::from_electrical_length(90.0, z0, freq);
    
    // Quarter-wave line transforms load impedance
    complex z_load(100.0, 0.0);
    complex z_in = qwt.input_impedance(z_load);
    
    // Z_in = Z0^2 / Z_load for quarter-wave
    complex expected = z0 * z0 / z_load;
    EXPECT_TRUE(complex_near(z_in, expected, 1.0));
}

// Test voltage and current gains
TEST(ConversionsTest, VoltageCurrentGains) {
    two_port xfmr = ideal_transformer(2.0);  // 2:1 step-down
    
    complex z_load(50.0, 0.0);
    
    // Voltage gain should be 1/n = 0.5
    complex v_gain = xfmr.voltage_gain(z_load);
    EXPECT_NEAR(v_gain.real(), 0.5, 1e-6);
    
    // Current gain should be n = 2.0
    complex i_gain = xfmr.current_gain(z_load);
    EXPECT_NEAR(i_gain.real(), 2.0, 1e-6);
}

// Test complex reference impedance
TEST(ConversionsTest, ComplexReferenceImpedance) {
    two_port r = series_resistor(25.0);
    
    // Use complex reference impedance
    complex z0(50.0, 10.0);
    auto s = r.to_s_parameters(z0);
    
    // Should still satisfy reciprocity
    complex det = s.determinant();
    EXPECT_LT(std::abs(det), 1.0);  // |det| < 1 for passive network
}

// Test characteristic impedance calculation
TEST(ConversionsTest, CharacteristicImpedance) {
    // For a symmetric T-network
    two_port r1 = series_resistor(25.0);
    two_port r2 = shunt_resistor(100.0);
    two_port r3 = series_resistor(25.0);
    
    two_port t_net = r1 * r2 * r3;
    
    // Should be symmetric (A = D)
    EXPECT_TRUE(t_net.is_symmetric(1e-10));
    
    // Calculate characteristic impedance
    complex z_char = t_net.characteristic_impedance();
    EXPECT_GT(z_char.real(), 0.0);
    EXPECT_NEAR(z_char.imag(), 0.0, 1e-10);
}

// Test parameter round-trip conversions
TEST(ConversionsTest, RoundTripConversions) {
    double freq = 2.4e9;
    two_port l = series_inductor(5e-9, freq);
    two_port c = shunt_capacitor(2e-12, freq);
    two_port r = series_resistor(10.0);
    
    two_port network = l * c * r;
    
    // Convert to S-parameters and check properties
    auto s = network.to_s_parameters(50.0);
    EXPECT_LT(std::abs(s.s11), 1.0);  // Passive
    EXPECT_LT(std::abs(s.s21), 1.0);  // Lossy
    
    // Convert to Z-parameters
    auto z = network.to_z_parameters();
    EXPECT_TRUE(complex_near(z.z12, z.z21));  // Reciprocal
    
    // Convert to Y-parameters
    auto y = network.to_y_parameters();
    EXPECT_TRUE(complex_near(y.y12, y.y21));  // Reciprocal
}

// Test S-parameters to ABCD conversion
TEST(ConversionsTest, SparamsToABCD) {
    // Create a known two-port (series 50Ω resistor)
    two_port original = series_resistor(50.0);
    double z0 = 50.0;
    
    // Convert to S-parameters
    auto s = original.to_s_parameters(z0);
    
    // Convert back to ABCD
    two_port converted = two_port::from_s_parameters(s, z0);
    
    // Should match original ABCD matrix
    EXPECT_TRUE(complex_near(original.a(), converted.a(), 1e-10));
    EXPECT_TRUE(complex_near(original.b(), converted.b(), 1e-10));
    EXPECT_TRUE(complex_near(original.c(), converted.c(), 1e-10));
    EXPECT_TRUE(complex_near(original.d(), converted.d(), 1e-10));
}

// Test S-parameters to ABCD conversion for reactive element
TEST(ConversionsTest, SparamsToABCDReactive) {
    double freq = 1e9;
    two_port original = series_inductor(10e-9, freq);  // 10nH at 1GHz
    double z0 = 50.0;
    
    // Convert to S-parameters
    auto s = original.to_s_parameters(z0);
    
    // Convert back to ABCD
    two_port converted = two_port::from_s_parameters(s, z0);
    
    // Should match original ABCD matrix
    EXPECT_TRUE(complex_near(original.a(), converted.a(), 1e-10));
    EXPECT_TRUE(complex_near(original.b(), converted.b(), 1e-10));
    EXPECT_TRUE(complex_near(original.c(), converted.c(), 1e-10));
    EXPECT_TRUE(complex_near(original.d(), converted.d(), 1e-10));
}

// Test S-parameters to ABCD conversion with complex reference impedance
TEST(ConversionsTest, SparamsToABCDComplexZ0) {
    two_port original = shunt_capacitor(1e-12, 2e9);  // 1pF at 2GHz
    complex z0(50.0, 10.0);  // Complex reference impedance
    
    // Convert to S-parameters
    auto s = original.to_s_parameters(z0);
    
    // Convert back to ABCD
    two_port converted = two_port::from_s_parameters(s, z0);
    
    // Should match original ABCD matrix
    EXPECT_TRUE(complex_near(original.a(), converted.a(), 1e-10));
    EXPECT_TRUE(complex_near(original.b(), converted.b(), 1e-10));
    EXPECT_TRUE(complex_near(original.c(), converted.c(), 1e-10));
    EXPECT_TRUE(complex_near(original.d(), converted.d(), 1e-10));
}

// Test reciprocity preservation in S->ABCD conversion
TEST(ConversionsTest, SparamsToABCDReciprocity) {
    // Create a cascaded network
    double freq = 1.5e9;
    auto l1 = series_inductor(5e-9, freq);
    auto c1 = shunt_capacitor(2e-12, freq);
    auto r1 = series_resistor(25.0);
    
    two_port original = l1 * c1 * r1;
    double z0 = 50.0;
    
    // Original should be reciprocal
    EXPECT_TRUE(original.is_reciprocal(1e-10));
    
    // Convert to S-parameters and back
    auto s = original.to_s_parameters(z0);
    two_port converted = two_port::from_s_parameters(s, z0);
    
    // Should still be reciprocal
    EXPECT_TRUE(converted.is_reciprocal(1e-10));
}