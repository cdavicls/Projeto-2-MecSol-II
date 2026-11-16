#pragma once

#include <cstddef>

namespace ms2 {

// Índice inteiro usado para identificar nós, elementos e graus de liberdade.
using Index = std::size_t;

// Tolerância numérica padrão para comparações e detecção de singularidade.
inline constexpr double kTolerance = 1e-12;

}
