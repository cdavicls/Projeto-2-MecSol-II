#include "ms2/Assembler.hpp"

#include <array>
#include <stdexcept>

namespace ms2 {

namespace {

// Retorna os quatro índices globais de DOF de um elemento, na ordem local
// [v_i, theta_i, v_j, theta_j].
std::array<Index, 4> elementDofs(const Element& element) {
    return {
        Model::dofV(element.nodeI()),
        Model::dofTheta(element.nodeI()),
        Model::dofV(element.nodeJ()),
        Model::dofTheta(element.nodeJ())
    };
}

// Soma um vetor de cargas locais no vetor global, usando o mapeamento de DOFs.
void addElementLoad(std::vector<double>& force,
                    const std::array<Index, 4>& dofs,
                    const std::array<double, 4>& local_load) {
    for (Index i = 0; i < 4; ++i) {
        force[dofs[i]] += local_load[i];
    }
}

}

Matrix Assembler::globalStiffness(const Model& model) {
    // A matriz global começa zerada e recebe a contribuição 4x4 de cada
    // elemento nas posições indicadas por elementDofs().
    const Index dof_count = model.dofCount();
    Matrix global(dof_count, dof_count);

    for (const Element& element : model.elements()) {
        const Matrix local = element.localStiffnessMatrix();
        const std::array<Index, 4> dofs = elementDofs(element);

        for (Index i = 0; i < 4; ++i) {
            for (Index j = 0; j < 4; ++j) {
                global(dofs[i], dofs[j]) += local(i, j);
            }
        }
    }

    return global;
}

std::vector<double> Assembler::globalLoadVector(const Model& model, const LoadCase& loads) {
    // O vetor global tem um valor por DOF. Cargas distribuídas e pontuais
    // são convertidas em cargas nodais equivalentes antes da soma.
    std::vector<double> force(model.dofCount(), 0.0);

    for (const NodalLoad& load : loads.nodal) {
        // Carga nodal entra diretamente nos DOFs de translação e rotação.
        if (load.node_id < 1 || load.node_id > model.nodeCount()) {
            throw std::invalid_argument("Nodal load references an invalid node");
        }
        force[Model::dofV(load.node_id)] += load.fy;
        force[Model::dofTheta(load.node_id)] += load.mz;
    }

    for (const DistributedLoad& load : loads.distributed) {
        // A carga q(x) é substituída pelas forças nodais equivalentes
        // obtidas pelas funções de forma do elemento.
        if (load.element_id < 1 || load.element_id > model.elementCount()) {
            throw std::invalid_argument("Distributed load references an invalid element");
        }
        const Element& element = model.element(load.element_id);
        const std::array<double, 4> local_load =
            element.equivalentDistributedLoad(load.q_i, load.q_j);
        addElementLoad(force, elementDofs(element), local_load);
    }

    for (const PointLoad& load : loads.point) {
        // Carga pontual também é distribuída aos nós via funções de forma.
        if (load.element_id < 1 || load.element_id > model.elementCount()) {
            throw std::invalid_argument("Point load references an invalid element");
        }
        const Element& element = model.element(load.element_id);
        const std::array<double, 4> local_load =
            element.equivalentPointLoad(load.x, load.fy);
        addElementLoad(force, elementDofs(element), local_load);
    }

    return force;
}

std::vector<double> Assembler::globalLoadVector(const Model& model,
                                                const std::vector<NodalLoad>& loads) {
    // Converte a lista de cargas nodais em um LoadCase e reutiliza a rotina
    // principal, evitando duplicação de lógica.
    LoadCase load_case;
    load_case.nodal = loads;
    return globalLoadVector(model, load_case);
}

}
