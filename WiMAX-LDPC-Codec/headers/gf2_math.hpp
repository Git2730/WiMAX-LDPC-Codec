#ifndef GF2_MATH_HPP
#define GF2_MATH_HPP

#include <vector>
#include <cstdint>
#include <stdexcept>

typedef std::vector<uint8_t> GF2Vector;
typedef std::vector<std::vector<uint8_t>> GF2Matrix;

class GF2Math {
public:
    //  Vector Addition (Hardware XOR)
    static GF2Vector add(const GF2Vector& a, const GF2Vector& b) {
        GF2Vector result(a.size());
        for (size_t i = 0; i < a.size(); ++i) {
            result[i] = a[i] ^ b[i]; // GF(2) addition is just XOR
        }
        return result;
    }

    //  Matrix Addition (Hardware XOR Array)
    static GF2Matrix add(const GF2Matrix& A, const GF2Matrix& B) {
        size_t rows = A.size();
        size_t cols = A[0].size();
        GF2Matrix result(rows, std::vector<uint8_t>(cols, 0));

        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                result[i][j] = A[i][j] ^ B[i][j];
            }
        }
        return result;
    }

    // Matrix-Vector Multiplication 
    static GF2Vector multiply(const GF2Matrix& M, const GF2Vector& v) {
        size_t rows = M.size();
        size_t cols = M[0].size();
        GF2Vector result(rows, 0);

        for (size_t i = 0; i < rows; ++i) {
            uint8_t sum = 0;
            for (size_t j = 0; j < cols; ++j) {
                sum ^= (M[i][j] & v[j]); // GF(2) Multiply is AND, Add is XOR
            }
            result[i] = sum;
        }
        return result;
    }

    // Matrix-Matrix Multiplication 
    static GF2Matrix multiply(const GF2Matrix& A, const GF2Matrix& B) {
        size_t rowsA = A.size();
        size_t colsA = A[0].size();
        size_t colsB = B[0].size();
        GF2Matrix result(rowsA, std::vector<uint8_t>(colsB, 0));
        
        for(size_t i = 0; i < rowsA; ++i) {
            for(size_t k = 0; k < colsA; ++k) {
                if (A[i][k]) { // Optimization: Only XOR if the multiplier is 1
                    for(size_t j = 0; j < colsB; ++j) {
                        result[i][j] ^= B[k][j];
                    }
                }
            }
        }
        return result;
    }

    // Forward-Substitution
    static GF2Vector forward_substitute(const GF2Matrix& T, const GF2Vector& x) {
        size_t n = T.size();
        GF2Vector y(n, 0);

        for (size_t i = 0; i < n; ++i) {
            uint8_t sum = x[i];

            for (size_t j = 0; j < i; ++j) {
                sum ^= (T[i][j] & y[j]);
            }
            y[i] = sum;
        }
        return y;
    }
    
    // Matrix version of Forward-Substitution (to calculate T_inv * A)
    static GF2Matrix forward_substitute_matrix(const GF2Matrix& T, const GF2Matrix& M) {
        size_t rows = M.size();
        size_t cols = M[0].size();
        GF2Matrix Y(rows, std::vector<uint8_t>(cols, 0));

        // Solve T * Y_col = M_col for every single column
        for (size_t j = 0; j < cols; ++j) {
            for (size_t i = 0; i < rows; ++i) {
                uint8_t sum = M[i][j];
                for (size_t k = 0; k < i; ++k) {
                    sum ^= (T[i][k] & Y[k][j]);
                }
                Y[i][j] = sum;
            }
        }
        return Y;
    }

    // Matrix Inversion (Gauss-Jordan Elimination)
    static GF2Matrix invert(GF2Matrix M) {
        size_t n = M.size();
        GF2Matrix inv(n, std::vector<uint8_t>(n, 0));
        
        for (size_t i = 0; i < n; i++) inv[i][i] = 1;

        for (size_t i = 0; i < n; i++) {
            // Find pivot
            if (M[i][i] == 0) {
                bool found = false;
                for (size_t k = i + 1; k < n; k++) {
                    if (M[k][i] == 1) {
                        std::swap(M[i], M[k]);
                        std::swap(inv[i], inv[k]);
                        found = true;
                        break;
                    }
                }
                if (!found) throw std::runtime_error("Matrix is singular and cannot be inverted!");
            }
            for (size_t k = 0; k < n; k++) {
                if (k != i && M[k][i] == 1) {
                    for (size_t j = 0; j < n; j++) {
                        M[k][j] ^= M[i][j];
                        inv[k][j] ^= inv[i][j];
                    }
                }
            }
        }
        return inv;
    }
};

#endif