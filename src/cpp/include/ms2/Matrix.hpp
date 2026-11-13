#pragma once

#include <vector>

#include "ms2/Types.hpp"

namespace ms2 {

class Matrix {
public:
    Matrix() = default;
    Matrix(Index rows, Index cols, double value = 0.0);

    static Matrix identity(Index size);

    Index rows() const;
    Index cols() const;
    bool empty() const;
    bool isSquare() const;
    bool isSymmetric(double tolerance = kTolerance) const;

    double& operator()(Index row, Index column);
    double operator()(Index row, Index column) const;

    std::vector<double>& data();
    const std::vector<double>& data() const;

    Matrix transposed() const;
    std::vector<double> multiply(const std::vector<double>& vector) const;

private:
    Index rows_ = 0;
    Index cols_ = 0;
    std::vector<double> data_;
};

}
