#include <iostream>
#include <iomanip>
#include <chrono>
#include "../include/cascadix.h"

using namespace cascadix;
using complex = std::complex<double>;

int main() {
    std::cout << "Cascadix Library - Filter Design Example with Frequency Sweeps\n";
    std::cout << "===============================================================\n\n";
    
    // Design parameters
    double fc = 1e9;  // 1 GHz cutoff
    double z0 = 50.0; // 50Ω system
    
    // Create 3rd order Butterworth lowpass filter
    std::cout << "3rd Order Butterworth Lowpass Filter\n";
    std::cout << "Cutoff frequency: " << fc/1e9 << " GHz\n";
    std::cout << "System impedance: " << z0 << " Ω\n\n";
    
    auto filter = make_butterworth_lc_lowpass_3rd(fc, z0);
    
    // Analyze at different frequencies
    double frequencies[] = {0.1e9, 0.5e9, 1.0e9, 2.0e9, 5.0e9};
    
    std::cout << std::setw(12) << "Freq (GHz)" 
              << std::setw(15) << "S11 (dB)" 
              << std::setw(15) << "S21 (dB)"
              << std::setw(15) << "VSWR"
              << std::setw(20) << "Input Z (Ω)\n";
    std::cout << std::string(80, '-') << "\n";
    
    for (double freq : frequencies) {
        // Recreate filter at each frequency
        double omega = 2.0 * PI * freq;
        double l_value = 0.7654 * z0 / (2.0 * PI * fc);
        double c_value = 1.8478 / (z0 * 2.0 * PI * fc);
        
        two_port l1 = series_inductor(l_value, freq);
        two_port c2 = shunt_capacitor(c_value, freq);
        two_port l3 = series_inductor(l_value, freq);
        
        two_port filter_at_freq = l1 * c2 * l3;
        
        // Calculate S-parameters
        auto s = filter_at_freq.to_s_parameters(z0);
        
        // Calculate metrics
        double s11_db = 20.0 * std::log10(std::abs(s.s11));
        double s21_db = 20.0 * std::log10(std::abs(s.s21));
        double vswr = s.vswr();
        complex z_in = filter_at_freq.input_impedance(z0);
        
        std::cout << std::fixed << std::setprecision(3);
        std::cout << std::setw(12) << freq/1e9
                  << std::setw(15) << s11_db
                  << std::setw(15) << s21_db
                  << std::setw(15) << vswr
                  << std::setw(20) << z_in << "\n";
    }
    
    std::cout << "\n";
    
    // NEW: Frequency sweep of the Butterworth filter
    std::cout << "Frequency Sweep of Butterworth Filter\n";
    std::cout << "=====================================\n";
    
    // Create a frequency sweep from 0.1 to 10 GHz
    frequency_sweep sweep(0.1e9, 10e9, 50, sweep_type::LOG);
    
    // Create a network builder for the Butterworth filter
    auto butterworth = make_butterworth_builder(fc, z0);
    
    // Perform the sweep
    auto sweep_results = perform_sweep(butterworth, sweep, z0);
    
    // Display results at key frequencies
    auto s21_db = sweep_results.get_s21_db();
    auto s11_db = sweep_results.get_s11_db();
    auto vswr = sweep_results.get_vswr();
    
    std::cout << std::setw(12) << "Freq (GHz)"
              << std::setw(12) << "S11 (dB)"
              << std::setw(12) << "S21 (dB)"
              << std::setw(12) << "VSWR\n";
    std::cout << std::string(48, '-') << "\n";
    
    // Display every 5th point for brevity
    for (size_t i = 0; i < sweep_results.frequencies.size(); i += 5) {
        std::cout << std::fixed << std::setprecision(3);
        std::cout << std::setw(12) << sweep_results.frequencies[i] / 1e9
                  << std::setw(12) << s11_db[i]
                  << std::setw(12) << s21_db[i]
                  << std::setw(12) << vswr[i] << "\n";
    }
    
    // Find 3dB point
    double cutoff_measured = 0;
    for (size_t i = 0; i < s21_db.size(); ++i) {
        if (s21_db[i] < -3.0) {
            cutoff_measured = sweep_results.frequencies[i];
            break;
        }
    }
    std::cout << "\nMeasured 3dB cutoff: " << cutoff_measured / 1e9 << " GHz\n";
    std::cout << "Design cutoff: " << fc / 1e9 << " GHz\n\n";
    
    // Example 2: Quarter-wave transformer
    std::cout << "Quarter-Wave Impedance Transformer\n";
    std::cout << "==================================\n";
    std::cout << "Transform 100Ω to 50Ω at 2.4 GHz\n\n";
    
    double f_design = 2.4e9;
    double z_load = 100.0;
    double z_source = 50.0;
    double z0_tline = std::sqrt(z_load * z_source);  // 70.7Ω
    
    two_port qwt = transmission_line::from_electrical_length(90.0, z0_tline, f_design);
    
    std::cout << "Transformer Z0: " << z0_tline << " Ω\n";
    
    // Check input impedance
    complex z_in_qwt = qwt.input_impedance(z_load);
    std::cout << "Input impedance with 100Ω load: " << z_in_qwt << " Ω\n";
    std::cout << "Target: " << z_source << " Ω\n\n";
    
    // Example 3: Pi attenuator
    std::cout << "3 dB Pi Attenuator\n";
    std::cout << "==================\n";
    
    auto atten = make_pi_attenuator(3.0, 50.0);
    auto s_atten = atten.to_s_parameters(50.0);
    
    std::cout << "S11: " << 20.0 * std::log10(std::abs(s_atten.s11)) << " dB\n";
    std::cout << "S21: " << 20.0 * std::log10(std::abs(s_atten.s21)) << " dB\n";
    std::cout << "Return Loss: " << s_atten.return_loss_db() << " dB\n";
    std::cout << "Insertion Loss: " << s_atten.insertion_loss_db() << " dB\n";
    std::cout << "VSWR: " << s_atten.vswr() << "\n\n";
    
    // Benchmark: Cascade performance
    std::cout << "Performance Benchmark\n";
    std::cout << "====================\n";
    
    // Warm up
    two_port chain = identity_two_port();
    for (int i = 0; i < 100; i++) {
        two_port r = series_resistor(1.0);
        chain = chain * r;
    }
    
    // Benchmark 1: Cascade 1000 resistors
    auto start = std::chrono::high_resolution_clock::now();
    chain = identity_two_port();
    for (int i = 0; i < 1000; i++) {
        two_port r = series_resistor(1.0);
        chain = chain * r;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Cascaded 1000 resistors in " << duration.count() << " μs\n";
    
    // Benchmark 2: Cascade 1000 frequency-dependent components
    double test_freq = 2.4e9;
    start = std::chrono::high_resolution_clock::now();
    chain = identity_two_port();
    for (int i = 0; i < 1000; i++) {
        two_port l = series_inductor(1e-9, test_freq);
        chain = chain * l;
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Cascaded 1000 inductors in " << duration.count() << " μs\n";
    
    // Benchmark 3: S-parameter calculations
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000; i++) {
        auto s = chain.to_s_parameters(50.0);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Calculated S-parameters 10000 times in " << duration.count() << " μs\n";
    std::cout << "Average: " << duration.count() / 10000.0 << " μs per calculation\n";
    
    return 0;
}