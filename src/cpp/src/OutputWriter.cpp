#include "ms2/OutputWriter.hpp"

#include <fstream>
#include <stdexcept>

#include "nlohmann/json.hpp"

namespace ms2 {

namespace {

using json = nlohmann::json;

}

std::string OutputWriter::toJson(const AnalysisResult& result) {
    json output;

    output["displacements"] = json::array();
    for (const NodalResult& node : result.nodes) {
        output["displacements"].push_back({
            {"node", node.node_id},
            {"v", node.v},
            {"theta", node.theta}
        });
    }

    output["reactions"] = json::array();
    for (const NodalResult& node : result.nodes) {
        output["reactions"].push_back({
            {"node", node.node_id},
            {"fy", node.reaction_fy},
            {"mz", node.reaction_mz}
        });
    }

    output["elements"] = json::array();
    for (const ElementResult& element : result.elements) {
        output["elements"].push_back({
            {"id", element.element_id},
            {"end_forces", {
                {"vi", element.end_forces[0]},
                {"mi", element.end_forces[1]},
                {"vj", element.end_forces[2]},
                {"mj", element.end_forces[3]}
            }},
            {"diagram", {
                {"x", element.x},
                {"shear", element.shear},
                {"moment", element.moment},
                {"deflection", element.deflection}
            }}
        });
    }

    output["maxima"] = {
        {"deflection", result.max_abs_deflection},
        {"shear", result.max_abs_shear},
        {"moment", result.max_abs_moment}
    };

    return output.dump(2);
}

void OutputWriter::writeFile(const std::string& path, const AnalysisResult& result) {
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open output file: " + path);
    }
    file << toJson(result) << '\n';
}

}
