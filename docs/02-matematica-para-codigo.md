---
type: content
status: active
updated: "2026-11-08"
tags: [projetos, pessoal]
---

# 02 — Matemática para código

Mapa entre as equações da formulação e as classes/métodos que as implementam.

→ [Voltar ao README](Home/Projetos/MS2_projeto2/README.md)

## Visão geral do fluxo

```text
InputParser::parseFile
        │
        ▼
BeamSolver::solve
        │
        ├── Assembler::globalStiffness
        ├── Assembler::globalLoadVector
        ├── BoundaryConditionHandler::reduce
        ├── LinearSolver::solve
        ├── BoundaryConditionHandler::expand
        └── PostProcessor::process
                │
                ▼
        OutputWriter::writeFile
```

## Mapa equação → código

| Equação / conceito | Implementação | Arquivo |
|---|---|---|
| Matriz de rigidez local, Eq. 32.17 | `Element::localStiffnessMatrix` | `src/cpp/src/Element.cpp:30` |
| Funções de forma de Hermite | `Element::shapeFunctions` | `src/cpp/src/Element.cpp:71` |
| Carga distribuída consistente | `Element::equivalentDistributedLoad` | `src/cpp/src/Element.cpp:91` |
| Carga pontual via funções de forma | `Element::equivalentPointLoad` | `src/cpp/src/Element.cpp:107` |
| Montagem de `K` | `Assembler::globalStiffness` | `src/cpp/src/Assembler.cpp:29` |
| Montagem de `F` com `LoadCase` | `Assembler::globalLoadVector` | `src/cpp/src/Assembler.cpp:47` |
| Montagem de `F` com cargas separadas | `Assembler::globalLoadVector` (sobrecarga) | `src/cpp/src/Assembler.cpp:81` |
| Partição do sistema e CCs | `BoundaryConditionHandler::reduce` | `src/cpp/src/BoundaryCondition.cpp:9` |
| Reconstrução do vetor global | `BoundaryConditionHandler::expand` | `src/cpp/src/BoundaryCondition.cpp:58` |
| Solução do sistema linear | `LinearSolver::solve` | `src/cpp/src/LinearSolver.cpp:10` |
| Reações, esforços e diagramas | `PostProcessor::process` | `src/cpp/src/PostProcessor.cpp:47` |
| Orquestração | `BeamSolver::solve` | `src/cpp/src/BeamSolver.cpp:8` e `:25` |
| Leitura do JSON | `InputParser::parseString` | `src/cpp/src/InputParser.cpp:29` |
| Escrita do JSON | `OutputWriter::toJson` | `src/cpp/src/OutputWriter.cpp:16` |
| Multiplicação matriz-vetor | `Matrix::multiply` | `src/cpp/src/Matrix.cpp:70` |

## Matriz de rigidez

`Element::localStiffnessMatrix()` monta a matriz 4×4 com `EI/L³` e os coeficientes `12`, `6L`, `4L²` e `2L²`. O método também valida `L > 0` e usa `Section::EI()`.

## Cargas equivalentes

`Element::equivalentDistributedLoad(q_i, q_j)` implementa a integração consistente:

```text
f_eq = ∫_0^L N(x)^T q(x) dx
```

`Element::equivalentPointLoad(x, fy)` calcula `N(x)^T P` e rejeita `x <= 0` ou `x >= L`.

## Montagem

`Assembler::globalStiffness(model)` percorre os elementos, obtém os DOFs globais e soma a matriz local nas posições correspondentes.

`Assembler::globalLoadVector(model, loads)` percorre:

1. `loads.nodal` → aplica diretamente nos DOFs do nó.
2. `loads.distributed` → calcula `f_eq` e soma nos DOFs do elemento.
3. `loads.point` → calcula `f_eq` e soma nos DOFs do elemento.

## Condições de contorno

`BoundaryConditionHandler::reduce` separa os DOFs livres e restritos. O sistema reduzido é resolvido e `expand` devolve o vetor global com zeros nos DOFs restritos. As reações são obtidas no pós-processamento por `R = K U - F`.

## Pós-processamento

`PostProcessor::process`:

1. Calcula `R = K U - F` nos DOFs restritos.
2. Para cada elemento, calcula `f_e = K_e q_e - f_eq`.
3. Gera amostras de `x` com `samples_per_element`.
4. Calcula `V(x)`, `M(x)` e a deformada integrada.
5. Agrega os máximos absolutos em `AnalysisResult::maxima`.

## Entrada e saída

`InputParser` valida IDs, conectividade, rigidez, apoios e cargas. `OutputWriter` serializa o `AnalysisResult` no contrato JSON usado pela CLI, pelo backend Python e pela GUI.

## Testes

- `tests/cpp/test_element.cpp` cobre matriz local, funções de forma e cargas equivalentes.
- `tests/cpp/test_cantilever.cpp` cobre o caso fechado de cantilever.
- `tests/cpp/test_examples.cpp` cobre os quatro exemplos de validação.
- `tests/cpp/test_io.cpp` cobre o contrato JSON e erros de validação.
- `tests/python/test_backend.py` cobre a API local e a integração com a CLI.
