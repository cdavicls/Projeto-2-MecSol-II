#pragma once

#include <vector>

#include "ms2/Types.hpp"

namespace ms2 {

// Carga aplicada diretamente em um nó.
struct NodalLoad {
    Index node_id = 0; // Nó onde a carga atua.
    double fy = 0.0;   // Força vertical (N); positivo para cima.
    double mz = 0.0;   // Momento fletor (N·m); positivo anti-horário.
};

// Carga distribuída linearmente ao longo de um elemento.
struct DistributedLoad {
    Index element_id = 0; // Elemento carregado.
    double q_i = 0.0;     // Intensidade no nó i (N/m).
    double q_j = 0.0;     // Intensidade no nó j (N/m).
};

// Carga pontual aplicada no interior de um elemento.
struct PointLoad {
    Index element_id = 0; // Elemento carregado.
    double x = 0.0;       // Posição local medida a partir do nó i (m).
    double fy = 0.0;      // Força vertical (N).
};

// Conjunto de todas as cargas de um caso de carregamento.
struct LoadCase {
    std::vector<NodalLoad> nodal;
    std::vector<DistributedLoad> distributed;
    std::vector<PointLoad> point;
};

}
