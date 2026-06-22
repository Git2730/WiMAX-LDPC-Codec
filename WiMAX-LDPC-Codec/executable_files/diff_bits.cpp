#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <iomanip>
#include "decoder.hpp"
#include "fixed_point_llr.hpp"

// Simulates BPSK transmission of all-zeros over an AWGN channel
// DOWNGRADED TO 5-BIT HARDWARE
std::vector<FixedPointLLR<5>> simulate_channel(double snr_db, int num_vars) {
    std::vector<FixedPointLLR<5>> channel_llrs;
    
    double R = 0.5; // WiMAX Rate 1/2
    double eb_no_linear = std::pow(10.0, snr_db / 10.0);
    double sigma = std::sqrt(1.0 / (2.0 * R * eb_no_linear));
    double sigma_sq = sigma * sigma;

    static std::mt19937 gen(42); 
    std::normal_distribution<double> d(0.0, sigma);

    for (int i = 0; i < num_vars; i++) {
        double tx_voltage = 1.0;
        double rx_voltage = tx_voltage + d(gen);
        double exact_llr = (2.0 * rx_voltage) / sigma_sq;
        
        double llr_std_dev = 2.0 / sigma;
        // Scale maps standard deviations to the 5-bit max limit (15)
        double scale = FixedPointLLR<5>::MAX_VAL / (4.0 * llr_std_dev);
        
        int quantized_llr = static_cast<int>(std::round(exact_llr * scale));
        channel_llrs.push_back(FixedPointLLR<5>(quantized_llr));
    }

    return channel_llrs;
}

void run_sanity_check() {
    std::cout << "=== SANITY CHECK ===" << std::endl;
    
    // LOCKED TO 5 ITERATIONS
    LDPCDecoder<5> decoder(5);
    int num_vars = 2304;
    
    // Test 1: Feed extremely confident all-zero LLRs
    // 15 is the physical maximum voltage for a 5-bit bus
    std::vector<FixedPointLLR<5>> perfect_llrs(num_vars, FixedPointLLR<5>(15));
    std::vector<int> result = decoder.decode(perfect_llrs);
    
    int errors = 0;
    for (int b : result) errors += b;
    
    std::cout << "Test 1 (Perfect +LLRs -> all zeros): " 
              << (errors == 0 ? "PASS" : "FAIL") 
              << " (" << errors << " errors)" << std::endl;

    // Test 2: Feed extremely confident all-one LLRs  
    // Every bit screams "I am definitely 1"
    //
    // NOTE: this H matrix has 4 base rows (of 12) with an ODD number of
    // connections -- rows 1, 2, 5, and 8 each have degree 7, every other
    // row has degree 6. For a linear code, the all-ones vector is a valid
    // codeword only if EVERY parity-check row has even weight (the XOR of
    // an odd count of 1s is always 1, never 0, so an odd-weight row can
    // never be satisfied by an all-ones input). Since 4 rows here are
    // odd-weight, the all-ones vector is PROVABLY NOT a codeword of this H
    // matrix -- no decoder, correct or not, can ever output it for this
    // input. So "ones == num_vars" was never a valid pass/fail criterion;
    // it was testing an impossible target, not the decoder. We instead
    // check that the result is heavily biased toward 1 (this still catches
    // a real sign-handling bug) and report how close it got.
    std::vector<FixedPointLLR<5>> negative_llrs(num_vars, FixedPointLLR<5>(-15));
    std::vector<int> result2 = decoder.decode(negative_llrs);
    
    int ones = 0;
    for (int b : result2) ones += b;
    
    std::cout << "Test 2 (Perfect -LLRs -> heavily biased to ones): "
              << (ones > num_vars / 2 ? "PASS" : "FAIL")
              << " (" << ones << "/" << num_vars << " ones -- all-ones itself "
              << "is not a valid codeword for this H, see comment above)" << std::endl;
              
    // Test 3: R_memory isolation — run test 1 twice
    // If R_memory bleeds between calls, second run will differ from first
    std::vector<int> result3 = decoder.decode(perfect_llrs);
    int errors3 = 0;
    for (int b : result3) errors3 += b;
    
    std::cout << "Test 3 (R_memory isolation):         "
              << (errors3 == 0 ? "PASS" : "FAIL")
              << " (should match Test 1)" << std::endl;
    std::cout << "====================" << std::endl << std::endl;
}

void run_monte_carlo() {
    std::vector<double> snr_points = {1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5};
    int max_frames_per_snr = 10000; 
    int bits_per_frame = 2304;

    // LOCKED TO 5 ITERATIONS
    LDPCDecoder<5> decoder(5);

    std::cout << "Starting WiMAX Rate-1/2 LDPC Hardware Simulation (5-Bit, 5 Iterations)..." << std::endl;
    std::cout << "--------------------------------------------------------------------------" << std::endl;
    std::cout << "SNR (dB) | Raw BER    | Decoded BER | FER        | Frames" << std::endl;
    std::cout << "--------------------------------------------------------------------------" << std::endl;

    for (size_t i = 0; i < snr_points.size(); i++) {
        double snr = snr_points[i];
        long long total_raw_errors = 0;
        long long total_decoded_errors = 0;
        int frame_errors = 0;
        int frames_run = 0;

        for (int f = 0; f < max_frames_per_snr; f++) {
            std::vector<FixedPointLLR<5>> channel_llrs = simulate_channel(snr, bits_per_frame);

            for (int b = 0; b < bits_per_frame; b++) {
                if (channel_llrs[b].value < 0) total_raw_errors++;
            }

            std::vector<int> output_bits = decoder.decode(channel_llrs);

            bool frame_failed = false;
            for (int b = 0; b < bits_per_frame; b++) {
                if (output_bits[b] == 1) {
                    total_decoded_errors++;
                    frame_failed = true;
                }
            }
            if (frame_failed) frame_errors++;
            frames_run++;

            // Statistical Stopping rule: 100 frame errors is reliable
            if (frame_errors >= 100) break;
        }

        double raw_ber = static_cast<double>(total_raw_errors) / (frames_run * bits_per_frame);
        double decoded_ber = static_cast<double>(total_decoded_errors) / (frames_run * bits_per_frame);
        double fer = static_cast<double>(frame_errors) / frames_run;

        std::cout << std::fixed << std::setprecision(1) << snr << "      | " 
                  << std::setprecision(6) << raw_ber << "   | " 
                  << decoded_ber << "    | " 
                  << fer << "    | " << frames_run << std::endl;
                  
        if (fer == 0) {
            std::cout << "\nZero errors achieved! The waterfall curve has bottomed out." << std::endl;
            break;
        }
    }
}

void run_error_correction_test() {
    std::cout << "=== ERROR CORRECTION TEST ===" << std::endl;
    
    // LOCKED TO 5 ITERATIONS
    LDPCDecoder<5> decoder(5);
    int num_vars = 2304;

    // Perfect all-zeros LLRs with exactly ONE bit flipped
    // A working LDPC decoder must fix a single bit error
    for (int test_bit = 0; test_bit < 10; test_bit++) {
        // Reduced from 50 to 15 to respect the 5-bit hardware maximum
        std::vector<FixedPointLLR<5>> llrs(num_vars, FixedPointLLR<5>(15));
        llrs[test_bit * 100] = FixedPointLLR<5>(-15); // flip one bit

        auto result = decoder.decode(llrs);
        int errors = 0;
        for (int b : result) errors += b;

        std::cout << "Bit " << test_bit * 100 << " error: "
                  << (errors == 0 ? "CORRECTED" : "FAILED")
                  << " (" << errors << " residual errors)" << std::endl;
    }
    std::cout << "=============================" << std::endl;
}

int main() {
    run_error_correction_test();
    run_sanity_check();
    run_monte_carlo();
    return 0;
}