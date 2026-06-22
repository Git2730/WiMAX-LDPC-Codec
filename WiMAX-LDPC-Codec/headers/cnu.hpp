#ifndef CNU_HPP
#define CNU_HPP

#include <vector>
#include <cstdint>
#include "signed_magnitude_llr.hpp"

template<int N_BITS>
class CheckNodeUnit {
public:
    // Process a single parity check equation
    static std::vector<SignedMagnitudeLLR<N_BITS>> process(const std::vector<SignedMagnitudeLLR<N_BITS>>& incoming) {
        int degree = incoming.size();
        std::vector<SignedMagnitudeLLR<N_BITS>> outgoing(degree);

        int overall_sign = 0;
        
        for(int i=0; i<incoming.size(); i++){
            overall_sign ^= incoming[i].sign_bit;
        }

        uint16_t min1 = SignedMagnitudeLLR<N_BITS>::allOnesMag(); 
        uint16_t min2 = SignedMagnitudeLLR<N_BITS>::allOnesMag();
        int min1_index = -1;

        for(int i=0; i<incoming.size(); i++){
            if(incoming[i].mag<min1){
                min2 = min1;
                min1 = incoming[i].mag;
                min1_index = i;
            }
            else if(incoming[i].mag<min2){
                min2 = incoming[i].mag;
            }
        }

        for (int i = 0; i < degree; i++) {
            // Outgoing sign: Overall sign XORed with this specific node's sign
            int out_sign = overall_sign ^ incoming[i].sign_bit;
            // Outgoing magnitude: min2 if this node provided min1, otherwise min1
            uint16_t out_mag = (i == min1_index) ? min2 : min1;
            outgoing[i] = SignedMagnitudeLLR<N_BITS>(out_sign, out_mag);
        }
        return outgoing;
    }

    // Process a parity equation using a physical Hardware Bypass Network (HBN)
    // 'incoming' and 'shifts' will always be the exact same size (fixed hardware width).
    static std::vector<SignedMagnitudeLLR<N_BITS>> processHBN(
        const std::vector<SignedMagnitudeLLR<N_BITS>>& incoming,
        const std::vector<int>& shifts) 
    {
        int degree = incoming.size();
        std::vector<SignedMagnitudeLLR<N_BITS>> outgoing(degree);

        // Create a local copy of the incoming messages to represent the masked signals
        std::vector<SignedMagnitudeLLR<N_BITS>> masked_incoming(degree);
        
        for(int i = 0; i < degree; i++) {
            if(shifts[i] == -1){
                masked_incoming[i] = SignedMagnitudeLLR<N_BITS>(0, SignedMagnitudeLLR<N_BITS>::allOnesMag());
            } else {
                masked_incoming[i] = incoming[i]; 
            }
        }

        int overall_sign = 0;
        uint16_t min1 = SignedMagnitudeLLR<N_BITS>::allOnesMag(); 
        uint16_t min2 = SignedMagnitudeLLR<N_BITS>::allOnesMag();
        int min1_index = -1;
        
        for(int i = 0; i < degree; i++) {
            overall_sign ^= masked_incoming[i].sign_bit;
            if(masked_incoming[i].mag < min1){
                min2 = min1;
                min1 = masked_incoming[i].mag;
                min1_index = i;
            }
            else if(masked_incoming[i].mag < min2){
                min2 = masked_incoming[i].mag;
            }
        }

        for (int i = 0; i < degree; i++) {
            if (shifts[i] == -1) {
                outgoing[i] = SignedMagnitudeLLR<N_BITS>(0, 0);
            } else {
                int out_sign = overall_sign ^ masked_incoming[i].sign_bit;
                uint16_t out_mag = (i == min1_index) ? min2 : min1;
                // out_mag = out_mag-out_mag >> 2;
                outgoing[i] = SignedMagnitudeLLR<N_BITS>(out_sign, out_mag);
            }
        }
        return outgoing;
    }
};

#endif