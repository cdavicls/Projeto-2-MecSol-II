#include <catch.hpp>

#include <stdexcept>
#include <string>

#include "ms2/BeamSolver.hpp"
#include "ms2/InputParser.hpp"
#include "ms2/OutputWriter.hpp"

TEST_CASE("JSON input parser and output writer", "[io]") {
    const std::string text = R"({
        "nodes": [
            {"id": 1, "x": 0.0, "y": 0.0},
            {"id": 2, "x": 4.0, "y": 0.0}
        ],
        "elements": [
            {"id": 1, "node_i": 1, "node_j": 2, "E": 210e9, "I": 8.333333333333333e-6}
        ],
        "supports": [
            {"node": 1, "fix_v": true},
            {"node": 2, "fix_v": true}
        ],
        "distributed_loads": [
            {"element": 1, "q_i": -1000.0, "q_j": -1000.0}
        ]
    })";

    const ms2::InputModel input = ms2::InputParser::parseString(text);

    REQUIRE(input.model.nodeCount() == 2);
    REQUIRE(input.model.elementCount() == 1);
    REQUIRE(input.conditions.size() == 2);
    REQUIRE(input.loads.distributed.size() == 1);

    const ms2::AnalysisResult result =
        ms2::BeamSolver::solve(input.model, input.loads, input.conditions);

    REQUIRE(result.nodes[0].reaction_fy == Approx(2000.0));

    const std::string output = ms2::OutputWriter::toJson(result);
    REQUIRE(output.find("\"displacements\"") != std::string::npos);
    REQUIRE(output.find("\"reactions\"") != std::string::npos);
    REQUIRE(output.find("\"elements\"") != std::string::npos);
    REQUIRE(output.find("\"maxima\"") != std::string::npos);
}

TEST_CASE("JSON parser rejects a point load at an element end", "[io]") {
    const std::string text = R"({
        "nodes": [
            {"id": 1, "x": 0.0, "y": 0.0},
            {"id": 2, "x": 4.0, "y": 0.0}
        ],
        "elements": [
            {"id": 1, "node_i": 1, "node_j": 2, "E": 210e9, "I": 8.333333333333333e-6}
        ],
        "point_loads": [
            {"element": 1, "x": 0.0, "fy": -1000.0}
        ]
    })";

    REQUIRE_THROWS_AS(ms2::InputParser::parseString(text), std::invalid_argument);
}
