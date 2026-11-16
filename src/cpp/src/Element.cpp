#include "ms2/Element.hpp"

#include <stdexcept>

namespace ms2 {

Element::Element(Index id, Index node_i, Index node_j, Section section)
    : id_(id), node_i_(node_i), node_j_(node_j), section_(section) {
    // O comprimento é calculado posteriormente pelo Model, quando as
    // coordenadas dos nós já estão disponíveis.
}

Index Element::id() const { return id_; }

Index Element::nodeI() const { return node_i_; }

Index Element::nodeJ() const { return node_j_; }

const Section& Element::section() const { return section_; }

Section& Element::section() { return section_; }

double Element::length() const { return length_; }

void Element::setLength(double length) {
    // Comprimento nulo ou negativo tornaria a matriz de rigidez inválida.
    if (length <= 0.0) {
        throw std::invalid_argument("Element length must be positive");
    }
    length_ = length;
}

Matrix Element::localStiffnessMatrix() const {
    if (length_ <= 0.0) {
        throw std::invalid_argument("Element length must be set before computing stiffness");
    }

    // Matriz de rigidez de viga de Euler-Bernoulli no sistema local.
    // Os DOFs são [v_i, theta_i, v_j, theta_j] e o fator comum é EI/L^3.
    const double L = length_;
    const double L2 = L * L;
    const double L3 = L2 * L;
    const double factor = section_.EI() / L3;

    Matrix stiffness(4, 4);

    // Linha/coluna do deslocamento vertical do nó i.
    stiffness(0, 0) = 12.0;
    stiffness(0, 1) = 6.0 * L;
    stiffness(0, 2) = -12.0;
    stiffness(0, 3) = 6.0 * L;

    // Linha/coluna da rotação do nó i.
    stiffness(1, 0) = 6.0 * L;
    stiffness(1, 1) = 4.0 * L2;
    stiffness(1, 2) = -6.0 * L;
    stiffness(1, 3) = 2.0 * L2;

    // Linha/coluna do deslocamento vertical do nó j.
    stiffness(2, 0) = -12.0;
    stiffness(2, 1) = -6.0 * L;
    stiffness(2, 2) = 12.0;
    stiffness(2, 3) = -6.0 * L;

    // Linha/coluna da rotação do nó j.
    stiffness(3, 0) = 6.0 * L;
    stiffness(3, 1) = 2.0 * L2;
    stiffness(3, 2) = -6.0 * L;
    stiffness(3, 3) = 4.0 * L2;

    // Aplica o fator EI/L^3 em todos os termos.
    for (Index i = 0; i < 4; ++i) {
        for (Index j = 0; j < 4; ++j) {
            stiffness(i, j) *= factor;
        }
    }

    return stiffness;
}

std::array<double, 4> Element::shapeFunctions(double x) const {
    // Funções de forma cúbicas de Hermite em coordenada local x.
    // Elas interpolam v e theta nas duas extremidades do elemento.
    if (length_ <= 0.0) {
        throw std::invalid_argument("Element length must be set before evaluating shape functions");
    }
    if (x < 0.0 || x > length_) {
        throw std::out_of_range("Local coordinate is outside the element");
    }

    // xi é a coordenada adimensional 0..1 ao longo do elemento.
    const double xi = x / length_;
    const double xi2 = xi * xi;
    const double xi3 = xi2 * xi;

    return {
        1.0 - 3.0 * xi2 + 2.0 * xi3,          // N1: v_i
        length_ * (xi - 2.0 * xi2 + xi3),     // N2: theta_i
        3.0 * xi2 - 2.0 * xi3,                // N3: v_j
        length_ * (-xi2 + xi3)                // N4: theta_j
    };
}

std::array<double, 4> Element::equivalentDistributedLoad(double q_i, double q_j) const {
    // Integra a carga linear q(x) contra as funções de forma para obter
    // as forças e momentos nodais equivalentes.
    if (length_ <= 0.0) {
        throw std::invalid_argument("Element length must be set before computing equivalent loads");
    }

    const double L = length_;
    const double L2 = L * L;

    return {
        (7.0 * q_i + 3.0 * q_j) * L / 20.0,
        (3.0 * q_i + 2.0 * q_j) * L2 / 60.0,
        (3.0 * q_i + 7.0 * q_j) * L / 20.0,
        -(2.0 * q_i + 3.0 * q_j) * L2 / 60.0
    };
}

std::array<double, 4> Element::equivalentPointLoad(double x, double fy) const {
    // Uma carga pontual é distribuída aos quatro DOFs usando o valor das
    // funções de forma na posição x.
    const std::array<double, 4> shape = shapeFunctions(x);
    return {
        shape[0] * fy,
        shape[1] * fy,
        shape[2] * fy,
        shape[3] * fy
    };
}

}
