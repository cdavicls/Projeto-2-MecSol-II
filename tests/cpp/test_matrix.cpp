#include <catch.hpp>

#include <stdexcept>

#include "ms2/Matrix.hpp"

TEST_CASE("Matrix stores and retrieves values", "[matrix]") {
    ms2::Matrix matrix(2, 3, 0.0);
    matrix(0, 0) = 1.0;
    matrix(0, 1) = 2.0;
    matrix(1, 2) = 5.0;

    REQUIRE(matrix.rows() == 2);
    REQUIRE(matrix.cols() == 3);
    REQUIRE(matrix(0, 0) == 1.0);
    REQUIRE(matrix(0, 1) == 2.0);
    REQUIRE(matrix(1, 2) == 5.0);
}

TEST_CASE("Matrix identity is square and symmetric", "[matrix]") {
    const ms2::Matrix identity = ms2::Matrix::identity(4);

    REQUIRE(identity.isSquare());
    REQUIRE(identity.isSymmetric());
    REQUIRE(identity(0, 0) == 1.0);
    REQUIRE(identity(3, 3) == 1.0);
    REQUIRE(identity(0, 3) == 0.0);
}

TEST_CASE("Matrix transpose swaps dimensions", "[matrix]") {
    ms2::Matrix matrix(2, 3, 0.0);
    matrix(0, 1) = 7.0;

    const ms2::Matrix transposed = matrix.transposed();

    REQUIRE(transposed.rows() == 3);
    REQUIRE(transposed.cols() == 2);
    REQUIRE(transposed(1, 0) == 7.0);
}

TEST_CASE("Matrix rejects out-of-range access", "[matrix]") {
    ms2::Matrix matrix(2, 2, 0.0);

    REQUIRE_THROWS_AS(matrix(2, 0), std::out_of_range);
    REQUIRE_THROWS_AS(matrix(0, 2), std::out_of_range);
}
