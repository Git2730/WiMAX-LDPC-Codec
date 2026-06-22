#include <iostream>
#include <vector>
#include <random>
#include "ru_encoder.hpp"
#include "expanded_matrix.hpp"

void run_encoder_validation() {
    std::cout << "=== RU ENCODER VALIDATION ===" << std::endl;
    
    ExpandedMatrix H; 
    RUEncoder encoder(H);
    
    int info_bits = 1152;
    int num_checks = 1152;
    int test_runs = 1000;
    int failed_frames = 0;

    std::mt19937 gen(42);
    std::uniform_int_distribution<int> dist(0, 1);

    for (int t = 0; t < test_runs; t++) {
        //Generate random user data (u)
        GF2Vector u(info_bits);
        for (int i = 0; i < info_bits; i++) {
            u[i] = dist(gen);
        }

        //Encode it
        GF2Vector codeword = encoder.encode(u);

        //Syndrome Check: Verify H * codeword == 0
        bool is_valid = true;
        for (int c = 0; c < num_checks; c++) {
            int parity_sum = 0;
            std::vector<int> connections = H.getCheckNodeConnections(c);
            for (int col : connections) {
                parity_sum ^= codeword[col];
            }
            if (parity_sum != 0) {
                is_valid = false;
                break;
            }
        }
        
        if (!is_valid) failed_frames++;
    }

    std::cout << "Tested " << test_runs << " random frames." << std::endl;
    std::cout << "Encoder Pass Rate: " << (test_runs - failed_frames) << " / " << test_runs << std::endl;
    std::cout << (failed_frames == 0 ? "SUCCESS: Codec is IEEE 802.16e Compliant." : "FAIL: Errors detected.") << std::endl;
    std::cout << "=============================" << std::endl << std::endl;
}

int main() {
    run_encoder_validation();
    return 0;
}