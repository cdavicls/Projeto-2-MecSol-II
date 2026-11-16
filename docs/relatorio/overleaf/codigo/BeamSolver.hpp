#pragma once

#include <vector>

#include "ms2/BoundaryCondition.hpp"
#include "ms2/Load.hpp"
#include "ms2/Model.hpp"
#include "ms2/PostProcessor.hpp"

namespace ms2 {

// Orquestra a análise: monta o sistema global, aplica as condições de
// contorno, resolve o sistema linear e calcula os resultados.
class BeamSolver {
public:
    // Resolve um caso de carregamento completo.
    // `samples_per_element` controla a quantidade de pontos dos diagramas.
    static AnalysisResult solve(const Model& model,
                                const LoadCase& loads,
                                const std::vector<BoundaryCondition>& conditions,
                                Index samples_per_element = 101);

    // Sobrecarga de conveniência para cargas apenas nodais.
    static AnalysisResult solve(const Model& model,
                                const std::vector<NodalLoad>& loads,
                                const std::vector<BoundaryCondition>& conditions);
};

}
