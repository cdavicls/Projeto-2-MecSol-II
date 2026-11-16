#pragma once

#include <string>

#include "ms2/PostProcessor.hpp"

namespace ms2 {

// Serializa os resultados da análise no formato JSON consumido pela GUI.
class OutputWriter {
public:
    // Converte o resultado completo para uma string JSON indentada.
    static std::string toJson(const AnalysisResult& result);

    // Grava o JSON em um arquivo.
    static void writeFile(const std::string& path, const AnalysisResult& result);
};

}
