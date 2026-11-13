#pragma once

#include <vector>

#include "ms2/BoundaryCondition.hpp"
#include "ms2/Load.hpp"
#include "ms2/Model.hpp"
#include "ms2/PostProcessor.hpp"

namespace ms2 {

class BeamSolver {
public:
    static AnalysisResult solve(const Model& model,
                                const LoadCase& loads,
                                const std::vector<BoundaryCondition>& conditions,
                                Index samples_per_element = 101);

    static AnalysisResult solve(const Model& model,
                                const std::vector<NodalLoad>& loads,
                                const std::vector<BoundaryCondition>& conditions);
};

}
