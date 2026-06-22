#ifndef SIGNED_MAGNITUDE_LLR_HPP
#define SIGNED_MAGNITUDE_LLR_HPP

#include <cstdint>
#include "fixed_point_llr.hpp"

template<int N_BITS>
struct SignedMagnitudeLLR {

    static constexpr uint16_t MAX_MAG = (1 << (N_BITS - 1)) - 1;

    int sign_bit;      // 0 = positive, 1 = negative
    uint16_t mag;      // magnitude in [0, MAX_MAG]

    SignedMagnitudeLLR() {
        sign_bit = 0;
        mag = 0;
    }

    SignedMagnitudeLLR(int sign, uint16_t magnitude) {
        sign_bit = sign;
        mag = (magnitude > MAX_MAG) ? MAX_MAG : magnitude;
    }

    uint16_t getMagnitude() const {
        return mag;
    }

    static uint16_t allOnesMag() {
        return MAX_MAG;
    }

    // C2S: Converts our Two's Complement LLR into Signed-Magnitude
    static SignedMagnitudeLLR fromTwosComplement(FixedPointLLR<N_BITS> tc) {
        int sign_bit=0;      // 0 = positive, 1 = negative
        uint16_t mag=tc.value;
        if(tc.value<0){
            sign_bit = 1;
            mag = -tc.value;
        }
        return SignedMagnitudeLLR<N_BITS>(sign_bit, mag);
    }

    // S2C: Converts Signed-Magnitude back into Two's Complement LLR
    static FixedPointLLR<N_BITS> toTwosComplement(SignedMagnitudeLLR sm) {
        int16_t value=sm.mag;
        if(sm.sign_bit == 1){
            value = -sm.mag;
        }
        return FixedPointLLR<N_BITS>(value);
    }
};
#endif