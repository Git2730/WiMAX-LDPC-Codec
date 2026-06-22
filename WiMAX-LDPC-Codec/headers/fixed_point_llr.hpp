#ifndef FIXED_POINT_LLR_HPP
#define FIXED_POINT_LLR_HPP

#include <cstdint>
#include <cmath>

template<int N_BITS>
struct FixedPointLLR {

    static_assert(N_BITS >= 3 && N_BITS <= 15, "N_BITS must be between 3 and 15");

    // --- Compile-time constants ---
    static constexpr int16_t MAX_VAL = (1 << (N_BITS - 1)) - 1;
    static constexpr int16_t MIN_VAL = -(1 << (N_BITS - 1));

    // --- Internal storage ---
    int16_t value;

    // --- Constructors ---
    FixedPointLLR() {
        value = 0; 
    }

    explicit FixedPointLLR(int x) {
        if (x > MAX_VAL) {
            value = MAX_VAL;
        } else if (x < MIN_VAL) {
            value = MIN_VAL;
        } else {
            value = x;
        }
    }

    explicit FixedPointLLR(double x) {
        int rounded = static_cast<int>(std::round(x));
        if (rounded > MAX_VAL) value = MAX_VAL;
        else if (rounded < MIN_VAL) value = MIN_VAL;
        else value = rounded;
    }

    FixedPointLLR operator+(const FixedPointLLR& rhs) const {
        int temp_sum = value + rhs.value;
        return FixedPointLLR<N_BITS>(temp_sum);
    }

    FixedPointLLR operator-(const FixedPointLLR& rhs) const {
        int temp_diff = value - rhs.value;
        return FixedPointLLR<N_BITS>(temp_diff);
    }

    FixedPointLLR operator-() const {
        int temp_neg = -value;
        return FixedPointLLR<N_BITS>(temp_neg);
    }

    bool operator>(const FixedPointLLR& rhs) const {
        return value>rhs.value;
    }

    bool operator<(const FixedPointLLR& rhs) const {
        return value<rhs.value;
    }

    bool operator==(const FixedPointLLR& rhs) const {
        return value==rhs.value;
    }

    double toFloat() const {
       return static_cast<double>(value);
    }

    int sign() const {
        if(value>=0){
            return 1;
        }
        return -1;
    }

    FixedPointLLR abs() const {
        return FixedPointLLR<N_BITS> (std::abs(value));
    }

    static FixedPointLLR maxMagnitude() {
        return FixedPointLLR<N_BITS>(MAX_VAL);
    }
};

#endif