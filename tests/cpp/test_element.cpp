#include <catch.hpp>

#include <stdexcept>

#include "ms2/Element.hpp"

TEST_CASE("Beam element stiffness matrix has expected values", "[element]") {
    const double E = 200.0e9;
    const double I = 1.0e-4;
    const double L = 2.0;

    ms2::Element element(1, 1, 2, ms2::Section{E, I});
    element.setLength(L);

    const ms2::Matrix stiffness = element.localStiffnessMatrix();
    const double factor = E * I / (L * L * L);

    REQUIRE(stiffness.rows() == 4);
    REQUIRE(stiffness.cols() == 4);
    REQUIRE(stiffness(0, 0) == Approx(12.0 * factor));
    REQUIRE(stiffness(0, 1) == Approx(6.0 * L * factor));
    REQUIRE(stiffness(0, 2) == Approx(-12.0 * factor));
    REQUIRE(stiffness(0, 3) == Approx(6.0 * L * factor));
    REQUIRE(stiffness(1, 1) == Approx(4.0 * L * L * factor));
    REQUIRE(stiffness(1, 3) == Approx(2.0 * L * L * factor));
    REQUIRE(stiffness(2, 2) == Approx(12.0 * factor));
    REQUIRE(stiffness(3, 3) == Approx(4.0 * L * L * factor));
}

TEST_CASE("Beam element stiffness matrix is symmetric", "[element]") {
    ms2::Element element(1, 1, 2, ms2::Section{210.0e9, 8.0e-6});
    element.setLength(3.0);

    REQUIRE(element.localStiffnessMatrix().isSymmetric());
}

TEST_CASE("Beam element rejects non-positive length", "[element]") {
    ms2::Element element(1, 1, 2, ms2::Section{210.0e9, 8.0e-6});

    REQUIRE_THROWS_AS(element.setLength(0.0), std::invalid_argument);
    REQUIRE_THROWS_AS(element.localStiffnessMatrix(), std::invalid_argument);
}

TEST_CASE("Beam element equivalent UDL vector", "[element][load]") {
    const double L = 3.0;
    const double w = 500.0;

    ms2::Element element(1, 1, 2, ms2::Section{210.0e9, 8.0e-6});
    element.setLength(L);

    const auto load = element.equivalentDistributedLoad(-w, -w);

    REQUIRE(load[0] == Approx(-w * L / 2.0));
    REQUIRE(load[1] == Approx(-w * L * L / 12.0));
    REQUIRE(load[2] == Approx(-w * L / 2.0));
    REQUIRE(load[3] == Approx(w * L * L / 12.0));
}

TEST_CASE("Beam element equivalent point load at midspan", "[element][load]") {
    const double L = 3.0;
    const double P = 1000.0;

    ms2::Element element(1, 1, 2, ms2::Section{210.0e9, 8.0e-6});
    element.setLength(L);

    const auto load = element.equivalentPointLoad(L / 2.0, -P);

    REQUIRE(load[0] == Approx(-P / 2.0));
    REQUIRE(load[1] == Approx(-P * L / 8.0));
    REQUIRE(load[2] == Approx(-P / 2.0));
    REQUIRE(load[3] == Approx(P * L / 8.0));
}
