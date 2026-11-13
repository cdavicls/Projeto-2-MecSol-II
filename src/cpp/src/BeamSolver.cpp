#include "ms2/BeamSolver.hpp"

#include "ms2/Assembler.hpp"
#include "ms2/LinearSolver.hpp"

namespace ms2 {

AnalysisResult BeamSolver::solve(const Model& model,
                                 const LoadCase& loads,
                                 const std::vector<BoundaryCondition>& conditions,
                                 Index samples_per_element) {
    const Matrix stiffness = Assembler::globalStiffness(model);
    const std::vector<double> force = Assembler::globalLoadVector(model, loads);
    const ReducedSystem reduced = BoundaryConditionHandler::reduce(
        stiffness, force, conditions, model.dofCount());
    const std::vector<double> free_solution = LinearSolver::solve(
        reduced.stiffness, reduced.force);
    const std::vector<double> displacements = BoundaryConditionHandler::expand(
        reduced, free_solution, model.dofCount());

    return PostProcessor::process(
        model, loads, conditions, stiffness, force, displacements, samples_per_element);
}

AnalysisResult BeamSolver::solve(const Model& model,
                                 const std::vector<NodalLoad>& loads,
                                 const std::vector<BoundaryCondition>& conditions) {
    LoadCase load_case;
    load_case.nodal = loads;
    return solve(model, load_case, conditions);
}

}
