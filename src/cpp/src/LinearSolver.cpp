#include "ms2/LinearSolver.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace ms2 {

std::vector<double> LinearSolver::solve(const Matrix& A, const std::vector<double>& b) {
    if (!A.isSquare()) {
        throw std::invalid_argument("Linear system matrix must be square");
    }
    if (A.rows() != b.size()) {
        throw std::invalid_argument("Linear system dimensions do not match");
    }

    const Index n = A.rows();
    if (n == 0) {
        return {};
    }

    Matrix a = A;
    std::vector<double> x = b;

    double scale = 0.0;
    for (Index i = 0; i < n; ++i) {
        for (Index j = 0; j < n; ++j) {
            scale = std::max(scale, std::abs(a(i, j)));
        }
    }
    if (scale == 0.0) {
        throw std::runtime_error("Linear system matrix is singular");
    }

    const double pivot_tolerance = kTolerance * scale;

    for (Index column = 0; column < n; ++column) {
        Index pivot = column;
        double pivot_abs = std::abs(a(column, column));

        for (Index row = column + 1; row < n; ++row) {
            const double candidate = std::abs(a(row, column));
            if (candidate > pivot_abs) {
                pivot_abs = candidate;
                pivot = row;
            }
        }

        if (pivot_abs <= pivot_tolerance) {
            throw std::runtime_error("Linear system matrix is singular or ill-conditioned");
        }

        if (pivot != column) {
            for (Index j = column; j < n; ++j) {
                std::swap(a(column, j), a(pivot, j));
            }
            std::swap(x[column], x[pivot]);
        }

        const double diagonal = a(column, column);
        for (Index row = column + 1; row < n; ++row) {
            const double factor = a(row, column) / diagonal;
            if (factor == 0.0) {
                continue;
            }
            a(row, column) = 0.0;
            for (Index j = column + 1; j < n; ++j) {
                a(row, j) -= factor * a(column, j);
            }
            x[row] -= factor * x[column];
        }
    }

    for (Index i = n; i-- > 0;) {
        double sum = x[i];
        for (Index j = i + 1; j < n; ++j) {
            sum -= a(i, j) * x[j];
        }
        x[i] = sum / a(i, i);
    }

    return x;
}

}
