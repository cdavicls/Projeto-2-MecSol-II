#pragma once

#include <vector>

#include "ms2/Types.hpp"

namespace ms2 {

struct NodalLoad {
    Index node_id = 0;
    double fy = 0.0;
    double mz = 0.0;
};

struct DistributedLoad {
    Index element_id = 0;
    double q_i = 0.0;
    double q_j = 0.0;
};

struct PointLoad {
    Index element_id = 0;
    double x = 0.0;
    double fy = 0.0;
};

struct LoadCase {
    std::vector<NodalLoad> nodal;
    std::vector<DistributedLoad> distributed;
    std::vector<PointLoad> point;
};

}
