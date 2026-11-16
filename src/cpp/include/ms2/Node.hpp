#pragma once

#include "ms2/Types.hpp"

namespace ms2 {

// Nó da estrutura. Cada nó possui dois graus de liberdade: v e theta.
struct Node {
    Index id = 0;  // Identificador sequencial, começando em 1.
    double x = 0.0; // Coordenada global x (m).
    double y = 0.0; // Coordenada global y (m).
};

}
