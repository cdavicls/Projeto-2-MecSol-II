#pragma once

#include <vector>

#include "ms2/Element.hpp"
#include "ms2/Node.hpp"
#include "ms2/Types.hpp"

namespace ms2 {

// Modelo estrutural completo: nós, elementos e a numeração dos graus de
// liberdade. A ordem dos DOFs é [v1, theta1, v2, theta2, ...].
class Model {
public:
    // Adiciona um nó; os ids precisam ser sequenciais e começar em 1.
    void addNode(const Node& node);

    // Adiciona um elemento e calcula automaticamente seu comprimento.
    void addElement(const Element& element);

    const std::vector<Node>& nodes() const;
    const std::vector<Element>& elements() const;

    // Acesso por id (1-based), com verificação de faixa.
    const Node& node(Index id) const;
    const Element& element(Index id) const;

    Index nodeCount() const;
    Index elementCount() const;

    // Cada nó contribui com dois DOFs: deslocamento v e rotação theta.
    Index dofCount() const { return 2 * nodeCount(); }

    // Índices globais dos DOFs de um nó.
    static Index dofV(Index node_id) { return 2 * (node_id - 1); }
    static Index dofTheta(Index node_id) { return 2 * (node_id - 1) + 1; }

private:
    std::vector<Node> nodes_;
    std::vector<Element> elements_;
};

}
