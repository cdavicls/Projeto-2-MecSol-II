#pragma once

#include <vector>

#include "ms2/Load.hpp"
#include "ms2/Matrix.hpp"
#include "ms2/Model.hpp"

namespace ms2 {

class Assembler {
public:
    static Matrix globalStiffness(const Model& model);
    static std::vector<double> globalLoadVector(const Model& model, const LoadCase& loads);
    static std::vector<double> globalLoadVector(const Model& model,
                                                const std::vector<NodalLoad>& loads);
};

}
