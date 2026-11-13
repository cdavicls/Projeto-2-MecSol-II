#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "ms2/BeamSolver.hpp"
#include "ms2/InputParser.hpp"
#include "ms2/OutputWriter.hpp"

namespace {

void printUsage() {
    std::cout
        << "Usage: ms2_cli [options]\n"
        << "\n"
        << "Options:\n"
        << "  -i, --input <file>   Read the model from a JSON file\n"
        << "  -o, --output <file>  Write the results to a JSON file (default: stdout)\n"
        << "  -s, --samples <n>    Samples per element for diagrams (default: 101)\n"
        << "  -h, --help           Show this help\n"
        << "\n"
        << "With no arguments, runs the built-in cantilever demo.\n";
}

int runDemo() {
    using namespace ms2;

    const double E = 210.0e9;
    const double I = 8.333333333333333e-6;
    const double L = 2.0;
    const double P = 1000.0;

    Model model;
    model.addNode(Node{1, 0.0, 0.0});
    model.addNode(Node{2, L, 0.0});
    model.addElement(Element{1, 1, 2, Section{E, I}});

    const std::vector<NodalLoad> loads{{2, -P, 0.0}};
    const std::vector<BoundaryCondition> conditions{{1, true, true}};

    const AnalysisResult result = BeamSolver::solve(model, loads, conditions);

    const double v_tip = result.displacements[Model::dofV(2)];
    const double theta_tip = result.displacements[Model::dofTheta(2)];
    const double v_exact = -P * L * L * L / (3.0 * E * I);
    const double theta_exact = -P * L * L / (2.0 * E * I);

    std::cout << std::scientific << std::setprecision(6);
    std::cout << "Cantilever tip displacement: " << v_tip << " m\n";
    std::cout << "Analytical displacement:     " << v_exact << " m\n";
    std::cout << "Cantilever tip rotation:     " << theta_tip << " rad\n";
    std::cout << "Analytical rotation:         " << theta_exact << " rad\n";
    std::cout << "Displacement error:          " << std::abs(v_tip - v_exact) << " m\n";
    std::cout << "Rotation error:              " << std::abs(theta_tip - theta_exact) << " rad\n";

    return 0;
}

}

int main(int argc, char** argv) {
    if (argc == 1) {
        return runDemo();
    }

    std::string input_path;
    std::string output_path;
    std::size_t samples_per_element = 101;

    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];

        if (argument == "-h" || argument == "--help") {
            printUsage();
            return 0;
        }
        if (argument == "-i" || argument == "--input") {
            if (i + 1 >= argc) {
                std::cerr << "Error: missing value for " << argument << "\n";
                return 1;
            }
            input_path = argv[++i];
            continue;
        }
        if (argument == "-o" || argument == "--output") {
            if (i + 1 >= argc) {
                std::cerr << "Error: missing value for " << argument << "\n";
                return 1;
            }
            output_path = argv[++i];
            continue;
        }
        if (argument == "-s" || argument == "--samples") {
            if (i + 1 >= argc) {
                std::cerr << "Error: missing value for " << argument << "\n";
                return 1;
            }
            try {
                const unsigned long parsed = std::stoul(argv[++i]);
                if (parsed < 2) {
                    std::cerr << "Error: samples per element must be at least 2\n";
                    return 1;
                }
                samples_per_element = static_cast<std::size_t>(parsed);
            } catch (const std::exception&) {
                std::cerr << "Error: invalid value for " << argument << "\n";
                return 1;
            }
            continue;
        }
        if (!argument.empty() && argument.front() != '-') {
            input_path = argument;
            continue;
        }

        std::cerr << "Error: unknown option " << argument << "\n";
        printUsage();
        return 1;
    }

    if (input_path.empty()) {
        std::cerr << "Error: no input file provided\n";
        printUsage();
        return 1;
    }

    try {
        const ms2::InputModel input = ms2::InputParser::parseFile(input_path);
        const ms2::AnalysisResult result = ms2::BeamSolver::solve(
            input.model, input.loads, input.conditions, samples_per_element);

        if (output_path.empty()) {
            std::cout << ms2::OutputWriter::toJson(result) << '\n';
        } else {
            ms2::OutputWriter::writeFile(output_path, result);
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
