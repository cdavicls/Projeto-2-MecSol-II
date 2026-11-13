#include <catch.hpp>

#include <cmath>
#include <cstddef>
#include <vector>

#include "ms2/BeamSolver.hpp"

namespace {

constexpr double kE = 210.0e9;
constexpr double kI = 8.333333333333333e-6;

std::size_t closestIndex(const std::vector<double>& values, double target) {
    std::size_t best = 0;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (std::abs(values[i] - target) < std::abs(values[best] - target)) {
            best = i;
        }
    }
    return best;
}

}

TEST_CASE("Cantilever under UDL matches analytical tip values", "[examples][udl]") {
    const double L = 4.0;
    const double w = 1000.0;

    ms2::Model model;
    model.addNode(ms2::Node{1, 0.0, 0.0});
    model.addNode(ms2::Node{2, L, 0.0});
    model.addElement(ms2::Element{1, 1, 2, ms2::Section{kE, kI}});

    ms2::LoadCase loads;
    loads.distributed.push_back(ms2::DistributedLoad{1, -w, -w});
    const std::vector<ms2::BoundaryCondition> conditions{{1, true, true}};

    const ms2::AnalysisResult result =
        ms2::BeamSolver::solve(model, loads, conditions);

    const double v_exact = -w * L * L * L * L / (8.0 * kE * kI);
    const double theta_exact = -w * L * L * L / (6.0 * kE * kI);

    REQUIRE(result.nodes[1].v == Approx(v_exact).epsilon(1e-10));
    REQUIRE(result.nodes[1].theta == Approx(theta_exact).epsilon(1e-10));
    REQUIRE(result.nodes[0].reaction_fy == Approx(w * L).epsilon(1e-10));
    REQUIRE(result.nodes[0].reaction_mz == Approx(w * L * L / 2.0).epsilon(1e-10));
}

TEST_CASE("Simply supported beam under UDL", "[examples][udl]") {
    const double L = 4.0;
    const double w = 1000.0;

    ms2::Model model;
    model.addNode(ms2::Node{1, 0.0, 0.0});
    model.addNode(ms2::Node{2, L, 0.0});
    model.addElement(ms2::Element{1, 1, 2, ms2::Section{kE, kI}});

    ms2::LoadCase loads;
    loads.distributed.push_back(ms2::DistributedLoad{1, -w, -w});
    const std::vector<ms2::BoundaryCondition> conditions{
        {1, true, false},
        {2, true, false}
    };

    const ms2::AnalysisResult result =
        ms2::BeamSolver::solve(model, loads, conditions);

    REQUIRE(result.nodes[0].reaction_fy == Approx(w * L / 2.0).epsilon(1e-10));
    REQUIRE(result.nodes[1].reaction_fy == Approx(w * L / 2.0).epsilon(1e-10));
    REQUIRE(result.nodes[0].reaction_mz == Approx(0.0).margin(1e-6));
    REQUIRE(result.nodes[1].reaction_mz == Approx(0.0).margin(1e-6));

    const ms2::ElementResult& element = result.elements.at(0);
    const std::size_t mid = element.x.size() / 2;

    REQUIRE(element.x.at(mid) == Approx(L / 2.0));
    REQUIRE(element.deflection.at(mid) ==
            Approx(-5.0 * w * L * L * L * L / (384.0 * kE * kI)).epsilon(1e-10));
    REQUIRE(element.moment.at(mid) == Approx(w * L * L / 8.0).epsilon(1e-10));
    REQUIRE(element.shear.front() == Approx(w * L / 2.0).epsilon(1e-10));
    REQUIRE(element.shear.back() == Approx(-w * L / 2.0).epsilon(1e-10));
    REQUIRE(element.moment.front() == Approx(0.0).margin(1e-6));
    REQUIRE(element.moment.back() == Approx(0.0).margin(1e-6));
}

TEST_CASE("Simply supported beam under triangular load", "[examples][triangular]") {
    const double L = 4.0;
    const double q0 = 1000.0;

    ms2::Model model;
    model.addNode(ms2::Node{1, 0.0, 0.0});
    model.addNode(ms2::Node{2, L, 0.0});
    model.addElement(ms2::Element{1, 1, 2, ms2::Section{kE, kI}});

    ms2::LoadCase loads;
    loads.distributed.push_back(ms2::DistributedLoad{1, 0.0, -q0});
    const std::vector<ms2::BoundaryCondition> conditions{
        {1, true, false},
        {2, true, false}
    };

    const ms2::AnalysisResult result =
        ms2::BeamSolver::solve(model, loads, conditions, 2001);

    REQUIRE(result.nodes[0].reaction_fy == Approx(q0 * L / 6.0).epsilon(1e-10));
    REQUIRE(result.nodes[1].reaction_fy == Approx(q0 * L / 3.0).epsilon(1e-10));

    const double moment_max = q0 * L * L / (9.0 * std::sqrt(3.0));
    REQUIRE(result.max_abs_moment == Approx(moment_max).epsilon(1e-6));
}

TEST_CASE("Two-span continuous beam under UDL", "[examples][continuous]") {
    const double L = 4.0;
    const double w = 1000.0;

    ms2::Model model;
    model.addNode(ms2::Node{1, 0.0, 0.0});
    model.addNode(ms2::Node{2, L, 0.0});
    model.addNode(ms2::Node{3, 2.0 * L, 0.0});
    model.addElement(ms2::Element{1, 1, 2, ms2::Section{kE, kI}});
    model.addElement(ms2::Element{2, 2, 3, ms2::Section{kE, kI}});

    ms2::LoadCase loads;
    loads.distributed.push_back(ms2::DistributedLoad{1, -w, -w});
    loads.distributed.push_back(ms2::DistributedLoad{2, -w, -w});
    const std::vector<ms2::BoundaryCondition> conditions{
        {1, true, false},
        {2, true, false},
        {3, true, false}
    };

    const ms2::AnalysisResult result =
        ms2::BeamSolver::solve(model, loads, conditions);

    REQUIRE(result.nodes[0].reaction_fy == Approx(3.0 * w * L / 8.0).epsilon(1e-10));
    REQUIRE(result.nodes[1].reaction_fy == Approx(10.0 * w * L / 8.0).epsilon(1e-10));
    REQUIRE(result.nodes[2].reaction_fy == Approx(3.0 * w * L / 8.0).epsilon(1e-10));

    const ms2::ElementResult& first = result.elements.at(0);
    REQUIRE(first.moment.back() == Approx(-w * L * L / 8.0).epsilon(1e-10));
}

TEST_CASE("Point load inside a simply supported element", "[examples][point]") {
    const double L = 4.0;
    const double P = 1000.0;

    ms2::Model model;
    model.addNode(ms2::Node{1, 0.0, 0.0});
    model.addNode(ms2::Node{2, L, 0.0});
    model.addElement(ms2::Element{1, 1, 2, ms2::Section{kE, kI}});

    ms2::LoadCase loads;
    loads.point.push_back(ms2::PointLoad{1, L / 2.0, -P});
    const std::vector<ms2::BoundaryCondition> conditions{
        {1, true, false},
        {2, true, false}
    };

    const ms2::AnalysisResult result =
        ms2::BeamSolver::solve(model, loads, conditions);

    REQUIRE(result.nodes[0].reaction_fy == Approx(P / 2.0).epsilon(1e-10));
    REQUIRE(result.nodes[1].reaction_fy == Approx(P / 2.0).epsilon(1e-10));

    const ms2::ElementResult& element = result.elements.at(0);
    const std::size_t index = closestIndex(element.x, L / 2.0);

    REQUIRE(element.x.at(index) == Approx(L / 2.0).margin(1e-12));
    REQUIRE(element.moment.at(index) == Approx(P * L / 4.0).epsilon(1e-10));
    REQUIRE(element.shear.at(index) == Approx(P / 2.0).epsilon(1e-10));
    REQUIRE(element.shear.at(index + 1) == Approx(-P / 2.0).epsilon(1e-10));
    REQUIRE(result.max_abs_moment == Approx(P * L / 4.0).epsilon(1e-10));
}
