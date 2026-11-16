#include "ms2/LinearSolver.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace ms2 {

std::vector<double> LinearSolver::solve(const Matrix& A, const std::vector<double>& b) {
    // Eliminação de Gauss com pivoteamento parcial.
    // A é copiada para não alterar a matriz global usada no pós-processamento.
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

    // Escala usada para uma tolerância de pivô relativa ao maior elemento.
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

    // Fase de eliminação: transforma A em uma matriz triangular superior.
    for (Index column = 0; column < n; ++column) {
        // Procura o maior pivô possível na coluna atual.
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

        // Troca as linhas do pivô, incluindo o termo correspondente em b.
        if (pivot != column) {
            for (Index j = column; j < n; ++j) {
                std::swap(a(column, j), a(pivot, j));
            }
            std::swap(x[column], x[pivot]);
        }

        // Elimina os termos abaixo da diagonal na coluna atual.
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

    // Fase de substituição regressiva: resolve de baixo para cima.
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
