#include "ms2/Model.hpp"

#include <cmath>
#include <stdexcept>

namespace ms2 {

void Model::addNode(const Node& node) {
    // A numeração sequencial simplifica o mapeamento id -> índice (id - 1).
    if (node.id != nodes_.size() + 1) {
        throw std::invalid_argument("Node ids must be contiguous and start at 1");
    }
    nodes_.push_back(node);
}

void Model::addElement(const Element& element) {
    // Valida a numeração do elemento e a existência dos nós conectados.
    if (element.id() != elements_.size() + 1) {
        throw std::invalid_argument("Element ids must be contiguous and start at 1");
    }
    if (element.nodeI() < 1 || element.nodeI() > nodes_.size() ||
        element.nodeJ() < 1 || element.nodeJ() > nodes_.size()) {
        throw std::invalid_argument("Element references an invalid node");
    }

    // O comprimento é a distância euclidiana entre os dois nós.
    Element stored = element;
    const Node& node_i = node(stored.nodeI());
    const Node& node_j = node(stored.nodeJ());
    const double dx = node_j.x - node_i.x;
    const double dy = node_j.y - node_i.y;
    stored.setLength(std::hypot(dx, dy));
    elements_.push_back(stored);
}

const std::vector<Node>& Model::nodes() const { return nodes_; }

const std::vector<Element>& Model::elements() const { return elements_; }

const Node& Model::node(Index id) const {
    // IDs são 1-based; o vetor interno é 0-based.
    if (id < 1 || id > nodes_.size()) {
        throw std::out_of_range("Node id out of range");
    }
    return nodes_[id - 1];
}

const Element& Model::element(Index id) const {
    // IDs são 1-based; o vetor interno é 0-based.
    if (id < 1 || id > elements_.size()) {
        throw std::out_of_range("Element id out of range");
    }
    return elements_[id - 1];
}

Index Model::nodeCount() const { return nodes_.size(); }

Index Model::elementCount() const { return elements_.size(); }

}
