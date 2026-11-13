#pragma once

#include <array>

#include "ms2/Matrix.hpp"
#include "ms2/Section.hpp"
#include "ms2/Types.hpp"

namespace ms2 {

class Element {
public:
    Element(Index id, Index node_i, Index node_j, Section section);

    Index id() const;
    Index nodeI() const;
    Index nodeJ() const;

    const Section& section() const;
    Section& section();

    double length() const;
    void setLength(double length);

    Matrix localStiffnessMatrix() const;

    std::array<double, 4> shapeFunctions(double x) const;
    std::array<double, 4> equivalentDistributedLoad(double q_i, double q_j) const;
    std::array<double, 4> equivalentPointLoad(double x, double fy) const;

private:
    Index id_ = 0;
    Index node_i_ = 0;
    Index node_j_ = 0;
    Section section_;
    double length_ = 0.0;
};

}
