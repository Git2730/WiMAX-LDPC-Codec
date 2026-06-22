#ifndef EXPANDED_MATRIX_HPP
#define EXPANDED_MATRIX_HPP

#include <vector>
#include "base_matrix.hpp"

class ExpandedMatrix {
private:
    BaseMatrix base;
    int Z;

public:
    ExpandedMatrix() {
        Z = BaseMatrix::Z; // 96
    }

    int getNumRows() const {
        return BaseMatrix::ROWS * Z; // 12 * 96 = 1152
    }

    int getNumCols() const {
        return BaseMatrix::COLS * Z; // 24 * 96 = 2304
    }

    // Returns the exact global Variable Nodes connected to a specific Check Node
    std::vector<int> getCheckNodeConnections(int check_node_idx) const {
        std::vector<int> connections;

        int base_row = check_node_idx / Z;
        int sub_row = check_node_idx % Z;

        std::vector<int> valid_base_cols = base.getConnections(base_row);

        for (int base_col : valid_base_cols) {
            int shift = base.get(base_row, base_col);
            int sub_col = (sub_row+shift) % Z;
            int absolute_col = base_col*Z + sub_col;
            
            connections.push_back(absolute_col);
        }
        return connections;
    }

    // Returns the exact global Check Nodes connected to a specific Variable Node
    std::vector<int> getVariableNodeConnections(int var_node_idx) const {
        std::vector<int> connected_checks;
        
        int total_check_nodes = getNumRows();
        for (int c = 0; c < total_check_nodes; c++) {
            std::vector<int> connected_vars = getCheckNodeConnections(c);
            
            for (size_t i = 0; i < connected_vars.size(); i++) {
                if (connected_vars[i] == var_node_idx) {
                    connected_checks.push_back(c);
                    break;
                }
            }
        }
        
        return connected_checks;
    }
};

#endif