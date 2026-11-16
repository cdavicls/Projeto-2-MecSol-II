---
type: content
status: active
updated: "2026-11-14"
tags: [projetos, pessoal]
---

# 01 — Teoria

Este documento fixa a formulação usada pelo solver. O objetivo é que qualquer resultado do código possa ser rastreado até a equação correspondente.

→ [Voltar ao README](Home/Projetos/MS2_projeto2/README.md)

## Hipóteses

- Viga de Euler-Bernoulli: seções planas permanecem planas e normais ao eixo deformado.
- Pequenas deformações e pequenos giros.
- Material elástico linear, `E` constante por elemento.
- Seção transversal constante por elemento, com momento de inércia `I`.
- Sem deformação por cisalhamento, sem efeitos axiais e sem torção.
- Viga horizontal; o eixo local `x` coincide com o eixo global `x`.

## Convenções de sinais

| Grandeza | Sinal positivo |
|---|---|
| Deslocamento vertical `v` | para cima |
| Giro `θ` | anti-horário |
| Carga distribuída `q` | para cima |
| Carga nodal `fy` | para cima |
| Momento nodal `mz` | anti-horário |
| Momento fletor `M` | traciona a fibra inferior (sagging) |
| Cortante `V` | gira o elemento no sentido anti-horário |

As convenções são travadas pelos quatro exemplos de validação. Qualquer alteração de sinal deve ser acompanhada da atualização dos `expected.json`.

## Equação governante

Para uma viga com rigidez `EI`:

```text
EI v''''(x) = q(x)
```

Integrando:

```text
EI v''(x) = M(x)
EI v'(x) = θ(x)
EI v(x)  = deslocamento vertical
```

## Elemento de viga

Cada nó possui dois graus de liberdade:

```text
q_e = [v_i, θ_i, v_j, θ_j]^T
```

A matriz de rigidez local é a Eq. (32.17):

```text
        EI   ⎡  12    6L   -12    6L ⎤
K_e = ─────  ⎢  6L   4L²   -6L   2L² ⎥
       L³    ⎢ -12   -6L    12   -6L ⎥
             ⎣  6L   2L²   -6L   4L² ⎦
```

No código: `Element::localStiffnessMatrix()` em `src/cpp/src/Element.cpp`.

## Funções de forma

Com `ξ = x/L`:

```text
N1(ξ) = 1 - 3ξ² + 2ξ³
N2(ξ) = L(ξ - 2ξ² + ξ³)
N3(ξ) = 3ξ² - 2ξ³
N4(ξ) = L(-ξ² + ξ³)
```

O deslocamento no interior do elemento é:

```text
v(ξ) = N1 v_i + N2 θ_i + N3 v_j + N4 θ_j
```

No código: `Element::shapeFunctions(double x)`.

## Carga distribuída linear

Para `q(ξ) = q_i(1-ξ) + q_j ξ`, o vetor de carga consistente é:

```text
f_eq = [ (7q_i + 3q_j)L/20,
         (q_i/20 + q_j/30)L²,
         (3q_i + 7q_j)L/20,
        -(q_i/30 + q_j/20)L² ]
```

Equivalente às expressões `(7q_i+3q_j)L/20`, `(3q_i+2q_j)L²/60`, `(3q_i+7q_j)L/20` e `-(2q_i+3q_j)L²/60`.

No código: `Element::equivalentDistributedLoad(double q_i, double q_j)`.

## Carga pontual interna

Uma carga `P` aplicada em `x = a` é distribuída aos DOFs por:

```text
f_eq = N(a)^T P
```

O ponto deve satisfazer `0 < a < L`. Nas extremidades, use `nodal_loads`.

No código: `Element::equivalentPointLoad(double x, double fy)`.

## Tipos de apoio

O JSON aceita um campo opcional `type` para tornar o apoio explícito:

| `type` | `fix_v` | `fix_theta` | Reação no modelo |
|---|---|---|---|
| `fixed` | `true` | `true` | `Fy` e `Mz` |
| `pinned` | `true` | `false` | apenas `Fy` |
| `roller` | `true` | `false` | apenas `Fy` |

Aliases em português: `engaste`, `pino` e `rolete`.

Como o elemento tem apenas os DOFs `v` e `θ`, não existe reação horizontal `Fx`. Pino e rolete são numericamente equivalentes; a diferença é apenas o símbolo no canvas. Os campos explícitos `fix_v` e `fix_theta`, quando presentes, sobrescrevem o atalho `type`.

No código: `InputParser::parseString` em `src/cpp/src/InputParser.cpp`.

## Montagem e solução

1. A matriz global `K` é montada pela conectividade dos elementos.
2. O vetor global `F` recebe cargas nodais e vetores equivalentes de cargas distribuídas e pontuais.
3. As condições de contorno são aplicadas por partição do sistema.
4. O sistema reduzido é resolvido por eliminação de Gauss com pivoteamento parcial.
5. As reações são calculadas por `R = K U - F`.
6. Os esforços de extremidade são `f_e = K_e q_e - f_eq`.

## Esforços internos

Para um elemento com carga distribuída linear e cargas pontuais internas:

```text
V(x) = F_i + q_i x + (q_j - q_i)x²/(2L) + Σ_{a<x} P

M(x) = -M_i + F_i x + q_i x²/2 + (q_j - q_i)x³/(6L) + Σ_{a<x} P(x-a)
```

A deformada é obtida integrando `EI v'' = M` exatamente dentro de cada elemento, com as condições de contorno locais `v(0)`, `θ(0)`, `v(L)` e `θ(L)`.

No código: `PostProcessor::process()` em `src/cpp/src/PostProcessor.cpp`.

## Limitações

- O modelo não possui DOF axial nem torcional.
- A viga é tratada como horizontal.
- Cargas pontuais devem estar no interior do elemento.
- O solver é estático e linear.
- O adaptador do formato Fortran do PDF ainda não foi implementado.
