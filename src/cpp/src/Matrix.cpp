#include "ms2/Matrix.hpp"

#include <cmath>
#include <stdexcept>

namespace ms2 {

Matrix::Matrix(Index rows, Index cols, double value)
    : rows_(rows), cols_(cols), data_(rows * cols, value) {
}

Matrix Matrix::identity(Index size) {
    Matrix result(size, size, 0.0);
    for (Index i = 0; i < size; ++i) {
        result(i, i) = 1.0;
    }
    return result;
}

Index Matrix::rows() const { return rows_; }

Index Matrix::cols() const { return cols_; }

bool Matrix::empty() const { return data_.empty(); }

bool Matrix::isSquare() const { return rows_ == cols_; }

bool Matrix::isSymmetric(double tolerance) const {
    if (!isSquare()) {
        return false;
    }
    for (Index i = 0; i < rows_; ++i) {
        for (Index j = i + 1; j < cols_; ++j) {
            if (std::abs((*this)(i, j) - (*this)(j, i)) > tolerance) {
                return false;
            }
        }
    }
    return true;
}

double& Matrix::operator()(Index row, Index column) {
    if (row >= rows_ || column >= cols_) {
        throw std::out_of_range("Matrix index out of range");
    }
    return data_[row * cols_ + column];
}

double Matrix::operator()(Index row, Index column) const {
    if (row >= rows_ || column >= cols_) {
        throw std::out_of_range("Matrix index out of range");
    }
    return data_[row * cols_ + column];
}

std::vector<double>& Matrix::data() { return data_; }

const std::vector<double>& Matrix::data() const { return data_; }

Matrix Matrix::transposed() const {
    Matrix result(cols_, rows_);
    for (Index i = 0; i < rows_; ++i) {
        for (Index j = 0; j < cols_; ++j) {
            result(j, i) = (*this)(i, j);
        }
    }
    return result;
}

std::vector<double> Matrix::multiply(const std::vector<double>& vector) const {
    if (vector.size() != cols_) {
        throw std::invalid_argument("Matrix-vector dimensions do not match");
    }

    std::vector<double> result(rows_, 0.0);
    for (Index i = 0; i < rows_; ++i) {
        for (Index j = 0; j < cols_; ++j) {
            result[i] += (*this)(i, j) * vector[j];
        }
    }
    return result;
}

}
