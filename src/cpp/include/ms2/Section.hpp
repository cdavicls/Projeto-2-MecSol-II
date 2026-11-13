#pragma once

namespace ms2 {

struct Section {
    double E = 0.0;
    double I = 0.0;

    double EI() const { return E * I; }
};

}
