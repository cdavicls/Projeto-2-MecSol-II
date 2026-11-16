#include "ms2/BeamSolver.hpp"

#include "ms2/Assembler.hpp"
#include "ms2/LinearSolver.hpp"

namespace ms2 {

AnalysisResult BeamSolver::solve(const Model& model,
                                 const LoadCase& loads,
                                 const std::vector<BoundaryCondition>& conditions,
                                 Index samples_per_element) {
    // 1. Monta o sistema global K u = F.
    const Matrix stiffness = Assembler::globalStiffness(model);
    const std::vector<double> force = Assembler::globalLoadVector(model, loads);

    // 2. Remove os DOFs restritos, formando o sistema reduzido.
    const ReducedSystem reduced = BoundaryConditionHandler::reduce(
        stiffness, force, conditions, model.dofCount());

    // 3. Resolve apenas os deslocamentos livres.
    const std::vector<double> free_solution = LinearSolver::solve(
        reduced.stiffness, reduced.force);

    // 4. Reconstrói o vetor global, com zero nos DOFs restritos.
    const std::vector<double> displacements = BoundaryConditionHandler::expand(
        reduced, free_solution, model.dofCount());

    // 5. Calcula reações, esforços internos e diagramas.
    return PostProcessor::process(
        model, loads, conditions, stiffness, force, displacements, samples_per_element);
}

AnalysisResult BeamSolver::solve(const Model& model,
                                 const std::vector<NodalLoad>& loads,
                                 const std::vector<BoundaryCondition>& conditions) {
    // Adapta a interface antiga (somente cargas nodais) para um LoadCase.
    LoadCase load_case;
    load_case.nodal = loads;
    return solve(model, load_case, conditions);
}

}
