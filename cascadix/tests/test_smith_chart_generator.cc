#include <gtest/gtest.h>
#include "../include/smith_chart_generator.h"
#include "../include/cascadix.h"
#include <cmath>

using namespace cascadix;
using complex = std::complex<double>;

// Helper function to check complex numbers are approximately equal
bool complex_near(const complex& a, const complex& b, double tolerance = 1e-6) {
    return std::abs(a - b) < tolerance;
}

// Test basic Smith chart transformations
TEST(SmithChartGeneratorTest, BasicTransformations) {
    double z0 = 50.0;
    
    // Test known impedance values
    complex z_matched(50.0, 0.0);    // Should map to origin
    complex z_open(1e12, 0.0);       // Should map to (1, 0)
    complex z_short(0.0, 0.0);       // Should map to (-1, 0)
    
    complex gamma_matched = smith_chart_generator::impedance_to_reflection(z_matched, z0);
    complex gamma_open = smith_chart_generator::impedance_to_reflection(z_open, z0);
    complex gamma_short = smith_chart_generator::impedance_to_reflection(z_short, z0);
    
    EXPECT_TRUE(complex_near(gamma_matched, complex(0.0, 0.0), 1e-10));
    EXPECT_NEAR(gamma_open.real(), 1.0, 1e-6);
    EXPECT_NEAR(gamma_open.imag(), 0.0, 1e-6);
    EXPECT_NEAR(gamma_short.real(), -1.0, 1e-10);
    EXPECT_NEAR(gamma_short.imag(), 0.0, 1e-10);
}

// Test round-trip conversion
TEST(SmithChartGeneratorTest, RoundTripConversion) {
    double z0 = 50.0;
    complex z_original(75.0, 25.0);  // Some arbitrary impedance
    
    complex gamma = smith_chart_generator::impedance_to_reflection(z_original, z0);
    complex z_recovered = smith_chart_generator::reflection_to_impedance(gamma, z0);
    
    EXPECT_TRUE(complex_near(z_original, z_recovered, 1e-10));
}

// Test point generation from simple network
TEST(SmithChartGeneratorTest, SimpleNetworkPoints) {
    smith_chart_generator generator;
    
    // Create a simple series resistor
    series_resistor resistor(25.0);  // 75Ω total with 50Ω reference
    
    // Create a frequency sweep
    frequency_sweep sweep(1e9, 2e9, 11, sweep_type::LINEAR);
    
    // Generate points
    std::vector<float> points = generator.generate_sweep_points(resistor, sweep, 50.0, 50.0);
    
    // Should have 11 points (22 floats)
    EXPECT_EQ(points.size(), 22);
    
    // All points should be the same (resistor doesn't change with frequency)
    for (size_t i = 0; i < points.size(); i += 2) {
        EXPECT_NEAR(points[i], points[0], 1e-10);      // Real parts equal
        EXPECT_NEAR(points[i+1], points[1], 1e-10);    // Imag parts equal
    }
}

// Test adaptive sampling with high-Q circuit
TEST(SmithChartGeneratorTest, AdaptiveSampling) {
    smith_chart_config config_adaptive;
    config_adaptive.adaptive_sampling = true;
    config_adaptive.min_spacing = 0.001;
    config_adaptive.max_spacing = 0.01;
    
    smith_chart_config config_uniform;
    config_uniform.adaptive_sampling = false;
    
    smith_chart_generator generator_adaptive(config_adaptive);
    smith_chart_generator generator_uniform(config_uniform);
    
    // Create a high-Q resonant circuit
    double freq = 1e9;
    auto inductor = series_inductor(10e-9, freq);  // 10nH
    auto capacitor = shunt_capacitor(2.5e-12, freq);  // Resonant at 1GHz
    auto network = inductor * capacitor;
    
    // Sweep around resonance with coarse frequency steps
    frequency_sweep sweep(0.95e9, 1.05e9, 11, sweep_type::LINEAR);
    
    auto points_adaptive = generator_adaptive.generate_sweep_points(network, sweep, 50.0, 50.0);
    auto points_uniform = generator_uniform.generate_sweep_points(network, sweep, 50.0, 50.0);
    
    // Adaptive sampling should generate more points due to rapid impedance changes
    EXPECT_GT(points_adaptive.size(), points_uniform.size());
    EXPECT_EQ(points_uniform.size(), 22);  // Should be exactly 11 points (22 floats)
}

// Test Monte Carlo point generation
TEST(SmithChartGeneratorTest, MonteCarloPoints) {
    smith_chart_generator generator;
    
    // Create some impedance values
    std::vector<complex> impedances = {
        complex(50.0, 0.0),
        complex(75.0, 25.0),
        complex(25.0, -25.0),
        complex(100.0, 50.0)
    };
    
    auto points = generator.generate_monte_carlo_points(impedances, 50.0);
    
    // Should have 4 points (8 floats)
    EXPECT_EQ(points.size(), 8);
    
    // All points should be within Smith chart bounds [-1, 1]
    for (size_t i = 0; i < points.size(); i++) {
        EXPECT_GE(points[i], -1.0);
        EXPECT_LE(points[i], 1.0);
    }
}

// Test edge density compensation
TEST(SmithChartGeneratorTest, EdgeDensityCompensation) {
    smith_chart_config config;
    config.min_spacing = 0.005;
    config.max_spacing = 0.02;
    config.edge_threshold = 0.7;
    config.edge_boost_factor = 3.0;
    
    smith_chart_generator generator(config);
    
    // Test spacing calculation for different radii
    complex gamma_center(0.1, 0.1);   // Near center
    complex gamma_edge(0.9, 0.1);     // Near edge
    
    double spacing_center = generator.calculate_point_spacing(gamma_center);
    double spacing_edge = generator.calculate_point_spacing(gamma_edge);
    
    // Edge should have smaller spacing (more points)
    EXPECT_GT(spacing_center, spacing_edge);
    EXPECT_NEAR(spacing_center, config.max_spacing, 0.005);
}

// Test S11 data import
TEST(SmithChartGeneratorTest, S11DataImport) {
    smith_chart_generator generator;
    
    // Create some S11 data (reflection coefficients)
    std::vector<complex> s11_data = {
        complex(0.0, 0.0),      // Matched
        complex(0.5, 0.0),      // Partial reflection
        complex(0.0, 0.5),      // Reactive mismatch
        complex(-0.3, -0.3)     // Complex reflection
    };
    
    auto points = generator.generate_from_s11_data(s11_data, 50.0);
    
    // Should have at least 4 points (8 floats)
    EXPECT_GE(points.size(), 8);
    
    // Check that first point matches input
    EXPECT_NEAR(points[0], 0.0, 1e-10);
    EXPECT_NEAR(points[1], 0.0, 1e-10);
}

// Test factory functions
TEST(SmithChartGeneratorTest, FactoryFunctions) {
    // Test network sweep
    series_inductor inductor(5e-9, 1e9);
    auto points = generate_network_sweep(inductor, 0.5e9, 1.5e9, 101, 50.0);
    
    EXPECT_GT(points.size(), 200);  // Should have at least 101 points
    
    // Test impedance cloud
    std::vector<complex> impedances = {
        complex(25.0, 0.0),
        complex(50.0, 0.0),
        complex(100.0, 0.0)
    };
    
    auto cloud_points = generate_impedance_cloud(impedances, 50.0);
    EXPECT_EQ(cloud_points.size(), 6);  // 3 points × 2 coordinates
}

// Test Monte Carlo sampler
TEST(SmithChartGeneratorTest, MonteCarloSampler) {
    monte_carlo_sampler sampler(12345);  // Fixed seed for reproducibility
    
    monte_carlo_sampler::component_variation resistor_var;
    resistor_var.nominal_value = 50.0;
    resistor_var.tolerance_percent = 5.0;  // 5% tolerance
    resistor_var.distribution = monte_carlo_sampler::component_variation::GAUSSIAN;
    
    auto samples = sampler.generate_samples(resistor_var, 1000);
    
    EXPECT_EQ(samples.size(), 1000);
    
    // Calculate statistics
    double mean = 0.0;
    for (double sample : samples) {
        mean += sample;
        EXPECT_GT(sample, 0.0);  // Should be positive
    }
    mean /= samples.size();
    
    // Mean should be close to nominal value
    EXPECT_NEAR(mean, 50.0, 1.0);
    
    // Calculate standard deviation
    double variance = 0.0;
    for (double sample : samples) {
        variance += (sample - mean) * (sample - mean);
    }
    variance /= (samples.size() - 1);
    double std_dev = std::sqrt(variance);
    
    // Standard deviation should be approximately tolerance/3
    double expected_std_dev = 50.0 * 0.05 / 3.0;  // ~0.83
    EXPECT_NEAR(std_dev, expected_std_dev, 0.3);
}

// Test impedance bounds clamping
TEST(SmithChartGeneratorTest, SmithChartBounds) {
    smith_chart_generator generator;
    
    // Create impedances that should map outside [-1, 1]
    std::vector<complex> extreme_impedances = {
        complex(1e-6, 0.0),     // Very low resistance
        complex(1e6, 0.0),      // Very high resistance
        complex(50.0, 1e6),     // Very high reactance
        complex(50.0, -1e6)     // Very high negative reactance
    };
    
    auto points = generator.impedances_to_smith_points(extreme_impedances, 50.0);
    
    // All points should be clamped to Smith chart bounds
    for (float coord : points) {
        EXPECT_GE(coord, -1.0);
        EXPECT_LE(coord, 1.0);
    }
}

// Test configuration changes
TEST(SmithChartGeneratorTest, ConfigurationChanges) {
    smith_chart_generator generator;
    
    // Test default configuration
    auto default_config = generator.get_config();
    EXPECT_TRUE(default_config.adaptive_sampling);
    EXPECT_GT(default_config.max_spacing, default_config.min_spacing);
    
    // Change configuration
    smith_chart_config new_config;
    new_config.min_spacing = 0.001;
    new_config.max_spacing = 0.005;
    new_config.adaptive_sampling = false;
    
    generator.set_config(new_config);
    auto updated_config = generator.get_config();
    
    EXPECT_FALSE(updated_config.adaptive_sampling);
    EXPECT_NEAR(updated_config.min_spacing, 0.001, 1e-10);
    EXPECT_NEAR(updated_config.max_spacing, 0.005, 1e-10);
}