#pragma once

#include <vector>

#include "ms2/Load.hpp"
#include "ms2/Matrix.hpp"
#include "ms2/Model.hpp"

namespace ms2 {

// Monta as matrizes e vetores globais a partir das contribuições locais
// de cada elemento.
class Assembler {
public:
    // Soma as matrizes de rigidez locais na matriz global.
    static Matrix globalStiffness(const Model& model);

    // Monta o vetor de cargas globais com cargas nodais, distribuídas e
    // pontuais convertidas para forças nodais equivalentes.
    static std::vector<double> globalLoadVector(const Model& model, const LoadCase& loads);

    // Sobrecarga de conveniência para casos com apenas cargas nodais.
    static std::vector<double> globalLoadVector(const Model& model,
                                                const std::vector<NodalLoad>& loads);
};

}
