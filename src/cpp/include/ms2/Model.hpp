#pragma once

#include <vector>

#include "ms2/Element.hpp"
#include "ms2/Node.hpp"
#include "ms2/Types.hpp"

namespace ms2 {

class Model {
public:
    void addNode(const Node& node);
    void addElement(const Element& element);

    const std::vector<Node>& nodes() const;
    const std::vector<Element>& elements() const;

    const Node& node(Index id) const;
    const Element& element(Index id) const;

    Index nodeCount() const;
    Index elementCount() const;
    Index dofCount() const { return 2 * nodeCount(); }

    static Index dofV(Index node_id) { return 2 * (node_id - 1); }
    static Index dofTheta(Index node_id) { return 2 * (node_id - 1) + 1; }

private:
    std::vector<Node> nodes_;
    std::vector<Element> elements_;
};

}
