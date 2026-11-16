---
type: content
status: active
updated: "2026-11-08"
tags: [projetos, pessoal]
---

# 06 — Exemplos e validação

Os quatro exemplos usam `E = 210 GPa`, `I = 8,333333333333333e-6 m⁴` e, portanto, `EI = 1,75e6 N·m²`.

Cada pasta em `examples/` segue o padrão `example-N/` e contém `input.json`, `expected.json` e `output.json`.

→ [Voltar ao README](Home/Projetos/MS2_projeto2/README.md)

## Como validar

```bash
./scripts/build.sh Debug
./scripts/validate_examples.py
```

O script executa a CLI para cada `input.json`, compara os campos de `expected.json` e imprime o erro relativo. A tolerância padrão é `1e-6`.

## Resumo

| # | Exemplo | Carregamento | Referência |
|---|---|---|---|
| 1 | Cantilever | Carga pontual `P = 1000 N` na ponta | `v = -PL³/(3EI)`, `θ = -PL²/(2EI)` |
| 2 | Simplesmente apoiada | UDL `q = 1000 N/m` para baixo | `v_mid = -5qL⁴/(384EI)`, `M_mid = qL²/8` |
| 3 | Simplesmente apoiada | Carga triangular `0 → -1000 N/m` | `R_A = q₀L/6`, `R_B = q₀L/3`, `M_max = q₀L²/(9√3)` |
| 4 | Viga contínua | UDL em dois vãos de 4 m | `R = 3qL/8`, `10qL/8`, `3qL/8` |

## Exemplo 1 — Cantilever

Entrada: `examples/example-1/input.json`.

| Grandeza | Analítico | Solver | Erro relativo |
|---|---:|---:|---:|
| `R_A` | 1000 N | 1000 N | 4,5e-16 |
| `M_A` | 4000 N·m | 4000 N·m | 1,1e-16 |
| `v_tip` | -0,0121904762 m | -0,0121904762 m | 1,4e-16 |
| `θ_tip` | -0,0045714286 rad | -0,0045714286 rad | 0 |
| `V_max` | 1000 N | 1000 N | 4,5e-16 |
| `M_max` | 4000 N·m | 4000 N·m | 1,1e-16 |

## Exemplo 2 — Simplesmente apoiada com UDL

Entrada: `examples/example-2/input.json`.

| Grandeza | Analítico | Solver | Erro relativo |
|---|---:|---:|---:|
| `R_A`, `R_B` | 2000 N | 2000 N | 0 |
| `θ_A` | -0,0015238095 rad | -0,0015238095 rad | 1,4e-16 |
| `θ_B` | 0,0015238095 rad | 0,0015238095 rad | 0 |
| `M_mid` | 2000 N·m | 2000 N·m | 0 |
| `v_mid` | -0,0019047619 m | -0,0019047619 m | 2,3e-16 |

## Exemplo 3 — Simplesmente apoiada com carga linear

Entrada: `examples/example-3/input.json`.

| Grandeza | Analítico | Solver | Erro relativo |
|---|---:|---:|---:|
| `R_A` | 666,6667 N | 666,6667 N | 1,7e-16 |
| `R_B` | 1333,3333 N | 1333,3333 N | 0 |
| `V_max` | 1333,3333 N | 1333,3333 N | 0 |
| `M_max` | 1026,4005 N·m | 1026,4005 N·m | 1,0e-7 |
| `v_max` | 0,0009541024 m | 0,0009541022 m | 1,4e-7 |

O momento máximo ocorre em `x = L/√3 ≈ 2,309 m`; a flecha máxima ocorre em `x ≈ 2,078 m` (cerca de `0,519L`). A pequena diferença vem da amostragem discreta do diagrama.

## Exemplo 4 — Viga contínua de dois vãos

Entrada: `examples/example-4/input.json`.

| Grandeza | Analítico | Solver | Erro relativo |
|---|---:|---:|---:|
| `R_A` | 1500 N | 1500 N | 0 |
| `R_B` | 5000 N | 5000 N | 0 |
| `R_C` | 1500 N | 1500 N | 0 |
| Momento no apoio interno | -2000 N·m | -2000 N·m | 0 |
| `V_max` | 2500 N | 2500 N | 0 |
| `M_max` | 2000 N·m | 2000 N·m | 0 |

## Testes automatizados

| Suíte | Conteúdo | Resultado |
|---|---|---|
| `ms2_tests` | 18 casos Catch2, 80 asserções | 100% |
| `ms2_backend` | 6 casos de API e integração | 100% |
| `validate_examples.py` | 26 verificações numéricas | todas `ok` |

## Interpretação dos erros

- Erros de `1e-16` são ruído de ponto flutuante.
- Erros de `1e-7` no exemplo 3 vêm da amostragem do máximo; aumentar `--samples` reduz a diferença.
- A validação usa erro relativo com denominador `max(|esperado|, 1)`.
