#pragma once

#include <vector>

#include "ms2/Matrix.hpp"
#include "ms2/Types.hpp"

namespace ms2 {

struct BoundaryCondition {
    Index node_id = 0;
    bool fix_v = false;
    bool fix_theta = false;
};

struct ReducedSystem {
    Matrix stiffness;
    std::vector<double> force;
    std::vector<Index> free_dofs;
    std::vector<Index> fixed_dofs;
};

class BoundaryConditionHandler {
public:
    static ReducedSystem reduce(const Matrix& global_stiffness,
                                const std::vector<double>& global_force,
                                const std::vector<BoundaryCondition>& conditions,
                                Index dof_count);

    static std::vector<double> expand(const ReducedSystem& reduced,
                                      const std::vector<double>& free_solution,
                                      Index dof_count);
};

}
