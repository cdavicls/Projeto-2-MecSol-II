#include "ms2/PostProcessor.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ms2 {

namespace {

std::vector<double> samplePositions(const Element& element,
                                    const std::vector<PointLoad>& point_loads,
                                    Index samples_per_element) {
    if (samples_per_element < 2) {
        throw std::invalid_argument("At least two samples per element are required");
    }

    const double L = element.length();
    std::vector<double> positions;
    positions.reserve(samples_per_element + 2 * point_loads.size());

    for (Index i = 0; i < samples_per_element; ++i) {
        const double ratio = static_cast<double>(i) / static_cast<double>(samples_per_element - 1);
        positions.push_back(L * ratio);
    }

    const double offset = L * 1e-9;
    for (const PointLoad& load : point_loads) {
        positions.push_back(load.x);
        if (load.x + offset < L) {
            positions.push_back(load.x + offset);
        }
    }

    std::sort(positions.begin(), positions.end());
    positions.erase(
        std::unique(positions.begin(), positions.end(), [](double a, double b) {
            return std::abs(a - b) < 1e-12;
        }),
        positions.end());

    return positions;
}

}

AnalysisResult PostProcessor::process(const Model& model,
                                      const LoadCase& loads,
                                      const std::vector<BoundaryCondition>& conditions,
                                      const Matrix& global_stiffness,
                                      const std::vector<double>& global_force,
                                      const std::vector<double>& displacements,
                                      Index samples_per_element) {
    const Index dof_count = model.dofCount();
    if (!global_stiffness.isSquare() || global_stiffness.rows() != dof_count) {
        throw std::invalid_argument("Global stiffness matrix has an invalid size");
    }
    if (global_force.size() != dof_count || displacements.size() != dof_count) {
        throw std::invalid_argument("Global vectors have an invalid size");
    }

    AnalysisResult result;
    result.displacements = displacements;

    std::vector<double> residual = global_stiffness.multiply(displacements);
    for (Index i = 0; i < dof_count; ++i) {
        residual[i] -= global_force[i];
    }

    std::vector<bool> fixed_v(dof_count, false);
    std::vector<bool> fixed_theta(dof_count, false);
    for (const BoundaryCondition& condition : conditions) {
        if (condition.node_id < 1 || condition.node_id > model.nodeCount()) {
            throw std::invalid_argument("Boundary condition references an invalid node");
        }
        if (condition.fix_v) {
            fixed_v[Model::dofV(condition.node_id)] = true;
        }
        if (condition.fix_theta) {
            fixed_theta[Model::dofTheta(condition.node_id)] = true;
        }
    }

    for (const Node& node : model.nodes()) {
        NodalResult nodal;
        nodal.node_id = node.id;
        nodal.v = displacements[Model::dofV(node.id)];
        nodal.theta = displacements[Model::dofTheta(node.id)];

        if (fixed_v[Model::dofV(node.id)]) {
            nodal.reaction_fy = residual[Model::dofV(node.id)];
        }
        if (fixed_theta[Model::dofTheta(node.id)]) {
            nodal.reaction_mz = residual[Model::dofTheta(node.id)];
        }

        result.nodes.push_back(nodal);
        result.max_abs_deflection =
            std::max(result.max_abs_deflection, std::abs(nodal.v));
    }

    for (const Element& element : model.elements()) {
        const Index element_id = element.id();
        const double L = element.length();
        const double EI = element.section().EI();
        if (EI <= 0.0) {
            throw std::invalid_argument("Element EI must be positive");
        }

        const std::array<Index, 4> dofs = {
            Model::dofV(element.nodeI()),
            Model::dofTheta(element.nodeI()),
            Model::dofV(element.nodeJ()),
            Model::dofTheta(element.nodeJ())
        };
        const std::array<double, 4> local_displacements = {
            displacements[dofs[0]],
            displacements[dofs[1]],
            displacements[dofs[2]],
            displacements[dofs[3]]
        };

        std::array<double, 4> equivalent_load{};
        double q_i = 0.0;
        double q_j = 0.0;
        std::vector<PointLoad> point_loads;

        for (const DistributedLoad& load : loads.distributed) {
            if (load.element_id != element_id) {
                continue;
            }
            const std::array<double, 4> local_load =
                element.equivalentDistributedLoad(load.q_i, load.q_j);
            for (Index k = 0; k < 4; ++k) {
                equivalent_load[k] += local_load[k];
            }
            q_i += load.q_i;
            q_j += load.q_j;
        }

        for (const PointLoad& load : loads.point) {
            if (load.element_id != element_id) {
                continue;
            }
            if (load.x <= 0.0 || load.x >= L) {
                throw std::invalid_argument(
                    "Point load must be strictly inside the element; use a nodal load at its ends");
            }
            const std::array<double, 4> local_load =
                element.equivalentPointLoad(load.x, load.fy);
            for (Index k = 0; k < 4; ++k) {
                equivalent_load[k] += local_load[k];
            }
            point_loads.push_back(load);
        }

        const Matrix local_stiffness = element.localStiffnessMatrix();
        const std::vector<double> local_force = local_stiffness.multiply(
            std::vector<double>(local_displacements.begin(), local_displacements.end()));

        ElementResult element_result;
        element_result.element_id = element_id;
        for (Index k = 0; k < 4; ++k) {
            element_result.end_forces[k] = local_force[k] - equivalent_load[k];
        }

        const double F_i = element_result.end_forces[0];
        const double M_i = element_result.end_forces[1];
        const double q_slope = (q_j - q_i) / L;

        element_result.x = samplePositions(element, point_loads, samples_per_element);
        element_result.shear.reserve(element_result.x.size());
        element_result.moment.reserve(element_result.x.size());
        element_result.deflection.reserve(element_result.x.size());

        for (const double x : element_result.x) {
            double shear = F_i + q_i * x + 0.5 * q_slope * x * x;
            double moment = -M_i + F_i * x + 0.5 * q_i * x * x
                          + q_slope * x * x * x / 6.0;

            double deflection = local_displacements[0] + local_displacements[1] * x;
            double deflection_term = -0.5 * M_i * x * x
                                   + F_i * x * x * x / 6.0
                                   + q_i * x * x * x * x / 24.0
                                   + q_slope * x * x * x * x * x / 120.0;

            for (const PointLoad& load : point_loads) {
                if (load.x < x) {
                    shear += load.fy;
                    moment += load.fy * (x - load.x);
                    const double distance = x - load.x;
                    deflection_term += load.fy * distance * distance * distance / 6.0;
                }
            }

            deflection += deflection_term / EI;

            element_result.shear.push_back(shear);
            element_result.moment.push_back(moment);
            element_result.deflection.push_back(deflection);

            result.max_abs_shear = std::max(result.max_abs_shear, std::abs(shear));
            result.max_abs_moment = std::max(result.max_abs_moment, std::abs(moment));
            result.max_abs_deflection =
                std::max(result.max_abs_deflection, std::abs(deflection));
        }

        result.elements.push_back(element_result);
    }

    return result;
}

}
