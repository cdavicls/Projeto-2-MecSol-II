#include "ms2/BoundaryCondition.hpp"

#include <stdexcept>

#include "ms2/Model.hpp"

namespace ms2 {

ReducedSystem BoundaryConditionHandler::reduce(const Matrix& global_stiffness,
                                               const std::vector<double>& global_force,
                                               const std::vector<BoundaryCondition>& conditions,
                                               Index dof_count) {
    if (!global_stiffness.isSquare() || global_stiffness.rows() != dof_count) {
        throw std::invalid_argument("Global stiffness matrix has an invalid size");
    }
    if (global_force.size() != dof_count) {
        throw std::invalid_argument("Global force vector has an invalid size");
    }

    std::vector<bool> fixed(dof_count, false);

    for (const BoundaryCondition& condition : conditions) {
        if (condition.node_id < 1 || condition.node_id > dof_count / 2) {
            throw std::invalid_argument("Boundary condition references an invalid node");
        }
        if (condition.fix_v) {
            fixed[Model::dofV(condition.node_id)] = true;
        }
        if (condition.fix_theta) {
            fixed[Model::dofTheta(condition.node_id)] = true;
        }
    }

    ReducedSystem reduced;
    for (Index dof = 0; dof < dof_count; ++dof) {
        if (fixed[dof]) {
            reduced.fixed_dofs.push_back(dof);
        } else {
            reduced.free_dofs.push_back(dof);
        }
    }

    const Index free_count = reduced.free_dofs.size();
    reduced.stiffness = Matrix(free_count, free_count);
    reduced.force.assign(free_count, 0.0);

    for (Index i = 0; i < free_count; ++i) {
        reduced.force[i] = global_force[reduced.free_dofs[i]];
        for (Index j = 0; j < free_count; ++j) {
            reduced.stiffness(i, j) = global_stiffness(reduced.free_dofs[i],
                                                       reduced.free_dofs[j]);
        }
    }

    return reduced;
}

std::vector<double> BoundaryConditionHandler::expand(const ReducedSystem& reduced,
                                                     const std::vector<double>& free_solution,
                                                     Index dof_count) {
    if (free_solution.size() != reduced.free_dofs.size()) {
        throw std::invalid_argument("Free solution vector has an invalid size");
    }

    std::vector<double> solution(dof_count, 0.0);
    for (Index i = 0; i < reduced.free_dofs.size(); ++i) {
        solution[reduced.free_dofs[i]] = free_solution[i];
    }

    return solution;
}

}
