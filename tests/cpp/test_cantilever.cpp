#include <catch.hpp>

#include <vector>

#include "ms2/BeamSolver.hpp"

TEST_CASE("Single-element cantilever matches analytical solution", "[cantilever]") {
    const double E = 210.0e9;
    const double I = 8.333333333333333e-6;
    const double L = 2.0;
    const double P = 1000.0;

    ms2::Model model;
    model.addNode(ms2::Node{1, 0.0, 0.0});
    model.addNode(ms2::Node{2, L, 0.0});
    model.addElement(ms2::Element{1, 1, 2, ms2::Section{E, I}});

    const std::vector<ms2::NodalLoad> loads{{2, -P, 0.0}};
    const std::vector<ms2::BoundaryCondition> conditions{{1, true, true}};

    const ms2::AnalysisResult result = ms2::BeamSolver::solve(model, loads, conditions);

    const double v_tip = result.displacements[ms2::Model::dofV(2)];
    const double theta_tip = result.displacements[ms2::Model::dofTheta(2)];
    const double v_exact = -P * L * L * L / (3.0 * E * I);
    const double theta_exact = -P * L * L / (2.0 * E * I);

    REQUIRE(v_tip == Approx(v_exact).epsilon(1e-10));
    REQUIRE(theta_tip == Approx(theta_exact).epsilon(1e-10));
    REQUIRE(result.displacements[ms2::Model::dofV(1)] == Approx(0.0));
    REQUIRE(result.displacements[ms2::Model::dofTheta(1)] == Approx(0.0));
}

TEST_CASE("Two-element cantilever matches analytical solution", "[cantilever]") {
    const double E = 210.0e9;
    const double I = 8.333333333333333e-6;
    const double L = 2.0;
    const double P = 1000.0;

    ms2::Model model;
    model.addNode(ms2::Node{1, 0.0, 0.0});
    model.addNode(ms2::Node{2, L / 2.0, 0.0});
    model.addNode(ms2::Node{3, L, 0.0});
    model.addElement(ms2::Element{1, 1, 2, ms2::Section{E, I}});
    model.addElement(ms2::Element{2, 2, 3, ms2::Section{E, I}});

    const std::vector<ms2::NodalLoad> loads{{3, -P, 0.0}};
    const std::vector<ms2::BoundaryCondition> conditions{{1, true, true}};

    const ms2::AnalysisResult result = ms2::BeamSolver::solve(model, loads, conditions);

    const double v_tip = result.displacements[ms2::Model::dofV(3)];
    const double v_exact = -P * L * L * L / (3.0 * E * I);

    REQUIRE(v_tip == Approx(v_exact).epsilon(1e-10));
}
