#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <iomanip>
#include "decoder.hpp"
#include "fixed_point_llr.hpp"
#include "expanded_matrix.hpp"
#include "ru_encoder.hpp"

// Simulates BPSK transmission of the encoded codeword over an AWGN channel
std::vector<FixedPointLLR<5>> simulate_channel(double snr_db, const GF2Vector& codeword) {
    std::vector<FixedPointLLR<5>> channel_llrs;
    int num_vars = codeword.size();
    
    double R = 0.5; // WiMAX Rate 1/2
    double eb_no_linear = std::pow(10.0, snr_db / 10.0);
    double sigma = std::sqrt(1.0 / (2.0 * R * eb_no_linear));
    double sigma_sq = sigma * sigma;

    static std::mt19937 noise_gen(42); 
    std::normal_distribution<double> d(0.0, sigma);

    for (int i = 0; i < num_vars; i++) {
        // BPSK Mapping: Bit 0 -> +1.0V, Bit 1 -> -1.0V
        double tx_voltage = (codeword[i] == 0) ? 1.0 : -1.0; 
        double rx_voltage = tx_voltage + d(noise_gen);
        
        // Exact LLR calculation
        double exact_llr = (2.0 * rx_voltage) / sigma_sq;
        
        double llr_std_dev = 2.0 / sigma;
        double scale = FixedPointLLR<5>::MAX_VAL / (4.0 * llr_std_dev);
        
        int quantized_llr = static_cast<int>(std::round(exact_llr * scale));
        channel_llrs.push_back(FixedPointLLR<5>(quantized_llr));
    }

    return channel_llrs;
}

void run_codec_pipeline() {
    std::vector<double> snr_points = {1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5};
    int max_frames_per_snr = 10000; 
    int bits_per_frame = 2304;
    int info_bits = 1152;

    // Initialize the hardware architecture
    ExpandedMatrix H;
    RUEncoder encoder(H);
    LDPCDecoder<5, 8> decoder(5); // 5-Bit Wires, 8-Bit Memory, 5 Iterations

    std::cout << "Starting Complete WiMAX Codec Pipeline (All-Zero Message)..." << std::endl;
    std::cout << "Hardware Constraints: 5-Bit Wires, 8-Bit Memory, 5 Iterations" << std::endl;
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
            
            // Create a blank user message
            GF2Vector u(info_bits, 0); 

            // Pass message through the RU encoder
            GF2Vector codeword = encoder.encode(u);

            // Transmit encoded data over noisy channel
            std::vector<FixedPointLLR<5>> channel_llrs = simulate_channel(snr, codeword);

            // Count raw channel errors
            for (int b = 0; b < bits_per_frame; b++) {
                int raw_hard_decision = (channel_llrs[b].value >= 0) ? 0 : 1;
                if (raw_hard_decision != codeword[b]) total_raw_errors++;
            }

            // Decode the noisy signals
            std::vector<int> output_bits = decoder.decode(channel_llrs);

            // Verify if the decoder successfully restored the original codeword
            bool frame_failed = false;
            for (int b = 0; b < bits_per_frame; b++) {
                if (output_bits[b] != codeword[b]) { 
                    total_decoded_errors++;
                    frame_failed = true;
                }
            }
            if (frame_failed) frame_errors++;
            frames_run++;

            // Stop testing if error threshold is reached
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

int main() {
    run_codec_pipeline();
    return 0;
}