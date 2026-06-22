#ifndef RU_ENCODER_HPP
#define RU_ENCODER_HPP

#include <iostream>
#include "expanded_matrix.hpp"
#include "gf2_math.hpp"

class RUEncoder {
private:
    GF2Matrix T_inv_A;
    GF2Matrix T_inv_B;
    GF2Matrix P0_matrix;

    GF2Matrix extractDenseSubMatrix(ExpandedMatrix& H, int r_start, int r_end, int c_start, int c_end) {
        int rows = r_end - r_start;
        int cols = c_end - c_start;
        GF2Matrix sub(rows, std::vector<uint8_t>(cols, 0));

        for (int r = r_start; r < r_end; ++r) {
            std::vector<int> connected_cols = H.getCheckNodeConnections(r);
            for (int c : connected_cols) {
                if (c >= c_start && c < c_end) {
                    sub[r - r_start][c - c_start] = 1;
                }
            }
        }
        return sub;
    }

public:
    RUEncoder(ExpandedMatrix& H) {
        int Z = 96;
        int num_rows = H.getNumRows(); // 1152
        int num_cols = H.getNumCols(); // 2304
        
        int row_split = 11 * Z;        // 1056
        int col_split_1 = 12 * Z;      // 1152
        int col_split_2 = 13 * Z;      // 1248
        
        std::cout << "Extracting sub-matrices..." << std::endl;
        GF2Matrix A = extractDenseSubMatrix(H, 0, row_split, 0, col_split_1);
        GF2Matrix B = extractDenseSubMatrix(H, 0, row_split, col_split_1, col_split_2);
        GF2Matrix T = extractDenseSubMatrix(H, 0, row_split, col_split_2, num_cols);
        GF2Matrix C = extractDenseSubMatrix(H, row_split, num_rows, 0, col_split_1);
        GF2Matrix D = extractDenseSubMatrix(H, row_split, num_rows, col_split_1, col_split_2);
        GF2Matrix E = extractDenseSubMatrix(H, row_split, num_rows, col_split_2, num_cols);

        std::cout << "Executing software pre-computations for hardware..." << std::endl;
        
        //Forward Substitution to bypass T inversion
        T_inv_A = GF2Math::forward_substitute_matrix(T, A);
        T_inv_B = GF2Math::forward_substitute_matrix(T, B);

        //Build Phi = (D + E * T_inv_B)
        GF2Matrix E_T_inv_B = GF2Math::multiply(E, T_inv_B);
        GF2Matrix Phi = GF2Math::add(D, E_T_inv_B);
        
        //Invert Phi
        GF2Matrix Phi_inv = GF2Math::invert(Phi);

        //Build the final P0 coefficient matrix: Phi_inv * (C + E * T_inv_A)
        GF2Matrix E_T_inv_A = GF2Math::multiply(E, T_inv_A);
        GF2Matrix right_term = GF2Math::add(C, E_T_inv_A);
        
        P0_matrix = GF2Math::multiply(Phi_inv, right_term);
        
        std::cout << "RU Encoder synthesis complete!" << std::endl;
    }

    GF2Vector encode(const GF2Vector& u) {
        // p0 = P0_matrix * u
        GF2Vector p0 = GF2Math::multiply(P0_matrix, u);

        // p1 = T_inv_A * u + T_inv_B * p0
        GF2Vector term1 = GF2Math::multiply(T_inv_A, u);
        GF2Vector term2 = GF2Math::multiply(T_inv_B, p0);
        GF2Vector p1 = GF2Math::add(term1, term2);

        // The final encoded message is [u p0 p1]
        GF2Vector codeword;
        codeword.reserve(u.size() + p0.size() + p1.size());
        codeword.insert(codeword.end(), u.begin(), u.end());
        codeword.insert(codeword.end(), p0.begin(), p0.end());
        codeword.insert(codeword.end(), p1.begin(), p1.end());

        return codeword;
    }
};

#endif