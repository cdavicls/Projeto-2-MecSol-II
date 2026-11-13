#pragma once

#include <string>

#include "ms2/BoundaryCondition.hpp"
#include "ms2/Load.hpp"
#include "ms2/Model.hpp"

namespace ms2 {

struct InputModel {
    Model model;
    LoadCase loads;
    std::vector<BoundaryCondition> conditions;
};

class InputParser {
public:
    static InputModel parseFile(const std::string& path);
    static InputModel parseString(const std::string& text);
};

}
