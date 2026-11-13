#pragma once

#include <vector>

#include "ms2/Matrix.hpp"

namespace ms2 {

class LinearSolver {
public:
    static std::vector<double> solve(const Matrix& A, const std::vector<double>& b);
};

}
