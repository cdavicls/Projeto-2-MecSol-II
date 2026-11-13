#pragma once

#include <string>

#include "ms2/PostProcessor.hpp"

namespace ms2 {

class OutputWriter {
public:
    static std::string toJson(const AnalysisResult& result);
    static void writeFile(const std::string& path, const AnalysisResult& result);
};

}
