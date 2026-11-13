#pragma once

#include <array>
#include <vector>

#include "ms2/BoundaryCondition.hpp"
#include "ms2/Load.hpp"
#include "ms2/Matrix.hpp"
#include "ms2/Model.hpp"

namespace ms2 {

struct NodalResult {
    Index node_id = 0;
    double v = 0.0;
    double theta = 0.0;
    double reaction_fy = 0.0;
    double reaction_mz = 0.0;
};

struct ElementResult {
    Index element_id = 0;
    std::array<double, 4> end_forces{};
    std::vector<double> x;
    std::vector<double> shear;
    std::vector<double> moment;
    std::vector<double> deflection;
};

struct AnalysisResult {
    std::vector<double> displacements;
    std::vector<NodalResult> nodes;
    std::vector<ElementResult> elements;
    double max_abs_deflection = 0.0;
    double max_abs_shear = 0.0;
    double max_abs_moment = 0.0;
};

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
