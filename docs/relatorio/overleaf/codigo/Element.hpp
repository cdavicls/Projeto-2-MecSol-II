#pragma once

#include <array>

#include "ms2/Matrix.hpp"
#include "ms2/Section.hpp"
#include "ms2/Types.hpp"

namespace ms2 {

// Elemento de viga de Euler-Bernoulli com dois nós e quatro DOFs locais:
// [v_i, theta_i, v_j, theta_j]. O comprimento é calculado pelo Model.
class Element {
public:
    Element(Index id, Index node_i, Index node_j, Section section);

    Index id() const;
    Index nodeI() const;
    Index nodeJ() const;

    const Section& section() const;
    Section& section();

    double length() const;

    // Define o comprimento; deve ser positivo.
    void setLength(double length);

    // Matriz de rigidez local 4x4 para flexão no plano.
    Matrix localStiffnessMatrix() const;

    // Funções de forma cúbicas de Hermite avaliadas em x local.
    // Retornam [N1, N2, N3, N4] multiplicando [v_i, theta_i, v_j, theta_j].
    std::array<double, 4> shapeFunctions(double x) const;

    // Cargas nodais equivalentes de uma carga distribuída linear q_i..q_j.
    std::array<double, 4> equivalentDistributedLoad(double q_i, double q_j) const;

    // Cargas nodais equivalentes de uma força pontual fy em x local.
    std::array<double, 4> equivalentPointLoad(double x, double fy) const;

private:
    Index id_ = 0;
    Index node_i_ = 0;
    Index node_j_ = 0;
    Section section_;
    double length_ = 0.0;
};

}
