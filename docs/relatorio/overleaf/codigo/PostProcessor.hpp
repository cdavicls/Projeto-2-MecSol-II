#pragma once

#include <array>
#include <vector>

#include "ms2/BoundaryCondition.hpp"
#include "ms2/Load.hpp"
#include "ms2/Matrix.hpp"
#include "ms2/Model.hpp"

namespace ms2 {

// Resultados nodais: deslocamentos, rotação e reações de apoio.
struct NodalResult {
    Index node_id = 0;
    double v = 0.0;          // Deslocamento transversal (m).
    double theta = 0.0;      // Rotação (rad).
    double reaction_fy = 0.0; // Reação vertical (N), se o DOF estiver restrito.
    double reaction_mz = 0.0; // Reação de momento (N·m), se restrito.
};

// Resultados de um elemento: esforços nas extremidades e diagramas amostrados.
struct ElementResult {
    Index element_id = 0;
    std::array<double, 4> end_forces{}; // [Vi, Mi, Vj, Mj] em coordenadas globais.
    std::vector<double> x;              // Posições locais dos diagramas (m).
    std::vector<double> shear;          // Cortante V(x) (N).
    std::vector<double> moment;         // Momento M(x) (N·m).
    std::vector<double> deflection;     // Deslocamento v(x) (m).
};

// Resultado completo de uma análise.
struct AnalysisResult {
    std::vector<double> displacements; // Vetor global de DOFs.
    std::vector<NodalResult> nodes;
    std::vector<ElementResult> elements;
    double max_abs_deflection = 0.0;
    double max_abs_shear = 0.0;
    double max_abs_moment = 0.0;
};

// Calcula reações, esforços internos e diagramas a partir dos deslocamentos.
class PostProcessor {
public:
    static AnalysisResult process(const Model& model,
                                  const LoadCase& loads,
                                  const std::vector<BoundaryCondition>& conditions,
                                  const Matrix& global_stiffness,
                                  const std::vector<double>& global_force,
                                  const std::vector<double>& displacements,
                                  Index samples_per_element = 101);
};

}
