#ifndef DECODER_HPP
#define DECODER_HPP

#include <vector>
#include "expanded_matrix.hpp"
#include "fixed_point_llr.hpp"
#include "signed_magnitude_llr.hpp"
#include "cnu.hpp"

template<int N_BITS, int ACC_BITS = 15>
class LDPCDecoder {
private:
    ExpandedMatrix H;
    int max_iterations;

    std::vector<FixedPointLLR<ACC_BITS>> Q_memory;
    std::vector<std::vector<FixedPointLLR<N_BITS>>> R_memory;

public:
    LDPCDecoder(int max_iter = 5) : max_iterations(max_iter) {
        int num_vars = H.getNumCols();
        int num_checks = H.getNumRows();

        Q_memory.assign(num_vars, FixedPointLLR<ACC_BITS>(0));
        R_memory.assign(num_checks, std::vector<FixedPointLLR<N_BITS>>(24, FixedPointLLR<N_BITS>(0)));
    }

    std::vector<int> decode(const std::vector<FixedPointLLR<N_BITS>>& channel_llrs) {
        int num_vars = H.getNumCols();
        int num_checks = H.getNumRows();
        int Z = 96;
        std::vector<int> hard_decisions(num_vars, 0);

        for (auto& row : R_memory) {
            std::fill(row.begin(), row.end(), FixedPointLLR<N_BITS>(0));
        }

        // Load initial channel LLRs
        for (int v = 0; v < num_vars; v++) {
            Q_memory[v] = FixedPointLLR<ACC_BITS>(static_cast<int>(channel_llrs[v].value));
        }

        for (int iter = 0; iter < max_iterations; iter++) {
            for (int c = 0; c < num_checks; c++) {
                std::vector<SignedMagnitudeLLR<N_BITS>> incoming(24, SignedMagnitudeLLR<N_BITS>(0, 0));
                std::vector<int> port_status(24, -1); 
                std::vector<int> connected_vars = H.getCheckNodeConnections(c);
                std::vector<long> q_ij_store(connected_vars.size(), 0);

                for (size_t i = 0; i < connected_vars.size(); i++) {
                    int v = connected_vars[i];
                    int p = v / Z;
                    port_status[p] = 1;

                    long q_ij_true = (long)Q_memory[v].value - (long)R_memory[c][p].value;
                    q_ij_store[i] = q_ij_true; 
                    
                    int q_ij_bus = (int)q_ij_true;
                    if (q_ij_bus > FixedPointLLR<N_BITS>::MAX_VAL) q_ij_bus = FixedPointLLR<N_BITS>::MAX_VAL;
                    if (q_ij_bus < -FixedPointLLR<N_BITS>::MAX_VAL) q_ij_bus = -FixedPointLLR<N_BITS>::MAX_VAL;

                    incoming[p] = SignedMagnitudeLLR<N_BITS>::fromTwosComplement(FixedPointLLR<N_BITS>(q_ij_bus));
                }

                std::vector<SignedMagnitudeLLR<N_BITS>> outgoing = CheckNodeUnit<N_BITS>::processHBN(incoming, port_status);
                
                for (size_t i = 0; i < connected_vars.size(); i++) {
                    int v = connected_vars[i];
                    int p = v / Z;

                    int r_new_val = SignedMagnitudeLLR<N_BITS>::toTwosComplement(outgoing[p]).value;
                    long new_q = q_ij_store[i] + (long)r_new_val;

                    Q_memory[v] = FixedPointLLR<ACC_BITS>((int)new_q);
                    R_memory[c][p] = FixedPointLLR<N_BITS>(r_new_val);
                }
            } 

            // Generate Hard Decisions
            for (int v = 0; v < num_vars; v++) {
                hard_decisions[v] = (Q_memory[v].value >= 0) ? 0 : 1;
            }

            //  EARLY EXIT CHECK 
            bool all_parity_checks_passed = true;
            for (int c_idx = 0; c_idx < num_checks; c_idx++) {
                int parity_sum = 0;
                std::vector<int> check_vars = H.getCheckNodeConnections(c_idx);
                for(size_t i = 0; i < check_vars.size(); i++){
                    parity_sum ^= hard_decisions[check_vars[i]];
                }
                if (parity_sum == 1) {
                    all_parity_checks_passed = false;
                    break;
                }
            }

            if (all_parity_checks_passed) break; 
        } 

        return hard_decisions;
    }
};

#endif