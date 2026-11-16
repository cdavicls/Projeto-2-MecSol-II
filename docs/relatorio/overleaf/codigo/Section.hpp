#pragma once

namespace ms2 {

// Propriedades da seção transversal de um elemento de viga.
struct Section {
    double E = 0.0; // Módulo de elasticidade (Pa).
    double I = 0.0; // Momento de inércia da seção (m^4).

    // Rigidez à flexão EI, usada na matriz de rigidez local.
    double EI() const { return E * I; }
};

}
