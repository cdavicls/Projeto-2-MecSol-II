#pragma once

#include <vector>

#include "ms2/Matrix.hpp"

namespace ms2 {

// Resolvedor de sistemas lineares densos.
class LinearSolver {
public:
    // Resolve A x = b por eliminação de Gauss com pivoteamento parcial.
    static std::vector<double> solve(const Matrix& A, const std::vector<double>& b);
};

}
