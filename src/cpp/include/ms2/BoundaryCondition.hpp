#pragma once

#include <vector>

#include "ms2/Matrix.hpp"
#include "ms2/Types.hpp"

namespace ms2 {

// Restrições aplicadas a um nó. Um engaste fixa v e theta; um apoio
// simples (pino ou rolete) fixa apenas v.
struct BoundaryCondition {
    Index node_id = 0;
    bool fix_v = false;     // Restringe o deslocamento transversal.
    bool fix_theta = false; // Restringe a rotação.
};

// Sistema linear após remover as linhas e colunas dos DOFs restritos.
struct ReducedSystem {
    Matrix stiffness;
    std::vector<double> force;
    std::vector<Index> free_dofs;  // DOFs que serão resolvidos.
    std::vector<Index> fixed_dofs; // DOFs prescritos (deslocamento nulo).
};

// Aplica as condições de contorno e reconstrói o vetor de deslocamentos.
class BoundaryConditionHandler {
public:
    // Monta o sistema reduzido contendo apenas os DOFs livres.
    static ReducedSystem reduce(const Matrix& global_stiffness,
                                const std::vector<double>& global_force,
                                const std::vector<BoundaryCondition>& conditions,
                                Index dof_count);

    // Expande a solução dos DOFs livres para o vetor global, preenchendo
    // os DOFs restritos com zero.
    static std::vector<double> expand(const ReducedSystem& reduced,
                                      const std::vector<double>& free_solution,
                                      Index dof_count);
};

}
