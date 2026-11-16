#pragma once

#include <string>

#include "ms2/BoundaryCondition.hpp"
#include "ms2/Load.hpp"
#include "ms2/Model.hpp"

namespace ms2 {

// Resultado do parsing: modelo, cargas e condições de contorno já
// convertidos para as estruturas internas do solver.
struct InputModel {
    Model model;
    LoadCase loads;
    std::vector<BoundaryCondition> conditions;
};

// Lê e valida modelos no formato JSON documentado em docs/04-api-e-formatos.md.
class InputParser {
public:
    // Lê o JSON de um arquivo.
    static InputModel parseFile(const std::string& path);

    // Lê o JSON já carregado em memória (usado pelo backend Python).
    static InputModel parseString(const std::string& text);
};

}
