#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "ms2/BeamSolver.hpp"
#include "ms2/InputParser.hpp"
#include "ms2/OutputWriter.hpp"

// Interface de linha de comando do solver.
// Sem argumentos, executa uma demonstração de viga em balanço; com --input ou
// --input-text, lê um modelo JSON, resolve e imprime/grava o resultado.

namespace {

void printUsage() {
    // Texto de ajuda exibido por --help e em erros de argumentos.
    std::cout
        << "Usage: ms2_cli [options]\n"
        << "\n"
        << "Options:\n"
        << "  -i, --input <file>        Read the model from a JSON file\n"
        << "  -it, --input-text <json>  Read the model from a JSON string\n"
        << "  -o, --output <file>       Write the results to a JSON file (default: stdout)\n"
        << "  -s, --samples <n>         Samples per element for diagrams (default: 101)\n"
        << "  -od, --only-displacements Print only nodal displacements to the terminal\n"
        << "  -h, --help                Show this help\n"
        << "\n"
        << "With no arguments, runs the built-in cantilever demo.\n";
}

int runDemo() {
    // Demonstração: viga em balanço com carga pontual na extremidade.
    // Compara o resultado numérico com a solução analítica de Euler-Bernoulli.
    using namespace ms2;

    const double E = 210.0e9;              // Aço (Pa).
    const double I = 8.333333333333333e-6; // Seção retangular (m^4).
    const double L = 2.0;                  // Comprimento (m).
    const double P = 1000.0;               // Carga na ponta (N).

    Model model;
    model.addNode(Node{1, 0.0, 0.0});
    model.addNode(Node{2, L, 0.0});
    model.addElement(Element{1, 1, 2, Section{E, I}});

    // Carga para baixo no nó 2 e engaste no nó 1.
    const std::vector<NodalLoad> loads{{2, -P, 0.0}};
    const std::vector<BoundaryCondition> conditions{{1, true, true}};

    const AnalysisResult result = BeamSolver::solve(model, loads, conditions);

    // Soluções analíticas para uma viga em balanço com carga na ponta.
    const double v_tip = result.displacements[Model::dofV(2)];
    const double theta_tip = result.displacements[Model::dofTheta(2)];
    const double v_exact = -P * L * L * L / (3.0 * E * I);
    const double theta_exact = -P * L * L / (2.0 * E * I);

    // Formata a saída em notação científica com seis casas decimais.
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "Cantilever tip displacement: " << v_tip << " m\n";
    std::cout << "Analytical displacement:     " << v_exact << " m\n";
    std::cout << "Cantilever tip rotation:     " << theta_tip << " rad\n";
    std::cout << "Analytical rotation:         " << theta_exact << " rad\n";
    std::cout << "Displacement error:          " << std::abs(v_tip - v_exact) << " m\n";
    std::cout << "Rotation error:              " << std::abs(theta_tip - theta_exact) << " rad\n";

    return 0;
}

void printDisplacements(const ms2::AnalysisResult& result) {
    // Saída compacta para inspeção rápida no terminal, sem o JSON completo.
    std::cout << std::scientific << std::setprecision(6);
    std::cout << "node" << std::setw(18) << "v (m)" << std::setw(20) << "theta (rad)" << '\n';
    for (const ms2::NodalResult& node : result.nodes) {
        std::cout << std::setw(4) << node.node_id
                  << std::setw(18) << node.v
                  << std::setw(20) << node.theta << '\n';
    }
}

}

int main(int argc, char** argv) {
    // Sem argumentos: executa a demonstração embutida.
    if (argc == 1) {
        return runDemo();
    }

    // Valores padrão dos argumentos de linha de comando.
    std::string input_path;
    std::string input_text;
    std::string output_path;
    std::size_t samples_per_element = 101;
    bool has_input_path = false;
    bool has_input_text = false;
    bool only_displacements = false;

    // Parser manual simples; cada opção consome o próximo argumento.
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];

        if (argument == "-h" || argument == "--help") {
            printUsage();
            return 0;
        }
        if (argument == "-it" || argument == "--input-text") {
            if (i + 1 >= argc) {
                std::cerr << "Error: missing value for " << argument << "\n";
                return 1;
            }
            input_text = argv[++i];
            has_input_text = true;
            continue;
        }
        if (argument == "-i" || argument == "--input") {
            if (i + 1 >= argc) {
                std::cerr << "Error: missing value for " << argument << "\n";
                return 1;
            }
            input_path = argv[++i];
            has_input_path = true;
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
        if (argument == "-od" || argument == "--only-displacements") {
            only_displacements = true;
            continue;
        }
        // Um argumento sem "-" é tratado como caminho de entrada posicional.
        if (!argument.empty() && argument.front() != '-') {
            input_path = argument;
            has_input_path = true;
            continue;
        }

        std::cerr << "Error: unknown option " << argument << "\n";
        printUsage();
        return 1;
    }

    //Tratamento de Entrada
    if (has_input_path && has_input_text) {
        std::cerr << "Error: use either --input or --input-text, not both\n";
        return 1;
    }
    if (!has_input_path && !has_input_text) {
        std::cerr << "Error: no input provided\n";
        printUsage();
        return 1;
    }

    try {
        // Lê o JSON do arquivo ou do texto, resolve o modelo e serializa o resultado.
        const ms2::InputModel input = has_input_text
            ? ms2::InputParser::parseString(input_text)
            : ms2::InputParser::parseFile(input_path);
        const ms2::AnalysisResult result = ms2::BeamSolver::solve(
            input.model, input.loads, input.conditions, samples_per_element);

        if (only_displacements) {
            // Modo compacto: imprime apenas v e theta por nó.
            printDisplacements(result);
        }
        if (!output_path.empty()) {
            // Com --output, o JSON completo é gravado no arquivo indicado.
            ms2::OutputWriter::writeFile(output_path, result);
        } else if (!only_displacements) {
            // Sem --output e sem --only-displacements, o JSON vai para stdout.
            std::cout << ms2::OutputWriter::toJson(result) << '\n';
        }
    } catch (const std::exception& error) {
        // Qualquer erro de parsing, validação ou resolução vira mensagem e
        // código de saída 1.
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
