#pragma once

#include <vector>

#include "ms2/Types.hpp"

namespace ms2 {

// Matriz densa de armazenamento linha por linha.
// Usada para a matriz de rigidez global, a matriz local do elemento
// e o sistema reduzido após a aplicação das condições de contorno.
class Matrix {
public:
    Matrix() = default;

    // Cria uma matriz rows x cols preenchida com `value`.
    Matrix(Index rows, Index cols, double value = 0.0);

    // Matriz identidade de ordem `size`.
    static Matrix identity(Index size);

    Index rows() const;
    Index cols() const;
    bool empty() const;
    bool isSquare() const;

    // Verifica se A(i,j) == A(j,i) dentro da tolerância informada.
    bool isSymmetric(double tolerance = kTolerance) const;

    // Acesso com verificação de limites; a versão não-const permite escrita.
    double& operator()(Index row, Index column);
    double operator()(Index row, Index column) const;

    std::vector<double>& data();
    const std::vector<double>& data() const;

    // Retorna a transposta da matriz.
    Matrix transposed() const;

    // Produto matriz-vetor, com verificação de dimensões.
    std::vector<double> multiply(const std::vector<double>& vector) const;

private:
    Index rows_ = 0;
    Index cols_ = 0;
    std::vector<double> data_;
};

}
