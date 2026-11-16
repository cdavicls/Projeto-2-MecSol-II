#include "ms2/InputParser.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "nlohmann/json.hpp"

namespace ms2 {

namespace {

using json = nlohmann::json;

// Garante que uma referência a nó exista antes de criar cargas/apoios.
void validateNode(const Model& model, Index node_id, const char* context) {
    if (node_id < 1 || node_id > model.nodeCount()) {
        throw std::invalid_argument(std::string(context) + " references an invalid node");
    }
}

// Garante que uma referência a elemento exista antes de criar cargas.
void validateElement(const Model& model, Index element_id, const char* context) {
    if (element_id < 1 || element_id > model.elementCount()) {
        throw std::invalid_argument(std::string(context) + " references an invalid element");
    }
}

}

InputModel InputParser::parseString(const std::string& text) {
    // O JSON é convertido diretamente para as estruturas do solver.
    // Erros de chave/tipo são propagados como exceções do nlohmann::json.
    const json document = json::parse(text);
    InputModel input;

    // Nós: id sequencial, coordenadas x/y em metros.
    for (const json& node : document.at("nodes")) {
        input.model.addNode(Node{
            node.at("id").get<Index>(),
            node.at("x").get<double>(),
            node.at("y").get<double>()
        });
    }

    // Elementos: conectividade e propriedades da seção (E, I).
    for (const json& element : document.at("elements")) {
        input.model.addElement(Element{
            element.at("id").get<Index>(),
            element.at("node_i").get<Index>(),
            element.at("node_j").get<Index>(),
            Section{element.at("E").get<double>(), element.at("I").get<double>()}
        });
    }

    // Apoios: `type` é um atalho para os dois flags de restrição.
    // `pinned` e `roller` são equivalentes numericamente neste modelo 2D
    // (ambos restringem apenas v); a diferença é apenas visual na GUI.
    if (document.contains("supports")) {
        for (const json& support : document.at("supports")) {
            const Index node_id = support.at("node").get<Index>();
            validateNode(input.model, node_id, "Support");

            bool fix_v = false;
            bool fix_theta = false;

            if (support.contains("type")) {
                const std::string type = support.at("type").get<std::string>();
                if (type == "fixed" || type == "engaste") {
                    // Engaste: restringe deslocamento e rotação.
                    fix_v = true;
                    fix_theta = true;
                } else if (type == "pinned" || type == "pin" ||
                           type == "pino" || type == "roller" ||
                           type == "rolete") {
                    // Pino/rolete: restringe apenas o deslocamento vertical.
                    fix_v = true;
                    fix_theta = false;
                } else {
                    throw std::invalid_argument("Unknown support type: " + type);
                }
            }

            // Campos explícitos têm prioridade sobre o atalho `type`.
            fix_v = support.value("fix_v", fix_v);
            fix_theta = support.value("fix_theta", fix_theta);

            input.conditions.push_back(BoundaryCondition{
                node_id,
                fix_v,
                fix_theta
            });
        }
    }

    // Cargas nodais: força vertical e momento aplicados diretamente no nó.
    if (document.contains("nodal_loads")) {
        for (const json& load : document.at("nodal_loads")) {
            const Index node_id = load.at("node").get<Index>();
            validateNode(input.model, node_id, "Nodal load");
            input.loads.nodal.push_back(NodalLoad{
                node_id,
                load.value("fy", 0.0),
                load.value("mz", 0.0)
            });
        }
    }

    // Cargas distribuídas: q_i e q_j definem a variação linear no elemento.
    if (document.contains("distributed_loads")) {
        for (const json& load : document.at("distributed_loads")) {
            const Index element_id = load.at("element").get<Index>();
            validateElement(input.model, element_id, "Distributed load");
            input.loads.distributed.push_back(DistributedLoad{
                element_id,
                load.value("q_i", 0.0),
                load.value("q_j", 0.0)
            });
        }
    }

    // Cargas pontuais: x é local ao elemento e deve ficar estritamente
    // dentro do intervalo (0, L). Nas extremidades, usar carga nodal.
    if (document.contains("point_loads")) {
        for (const json& load : document.at("point_loads")) {
            const Index element_id = load.at("element").get<Index>();
            validateElement(input.model, element_id, "Point load");
            const double x = load.at("x").get<double>();
            const Element& element = input.model.element(element_id);
            if (x <= 0.0 || x >= element.length()) {
                throw std::invalid_argument(
                    "Point load x must be strictly inside the element; use a nodal load at its ends");
            }
            input.loads.point.push_back(PointLoad{
                element_id,
                x,
                load.value("fy", 0.0)
            });
        }
    }

    return input;
}

InputModel InputParser::parseFile(const std::string& path) {
    // Lê o arquivo inteiro para uma string e reutiliza parseString().
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open input file: " + path);
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return parseString(buffer.str());
}

}
