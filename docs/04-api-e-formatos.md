---
type: content
status: active
updated: "2026-11-08"
tags: [projetos, pessoal]
---

# 04 — API e formatos

Contrato JSON, CLI e API HTTP local.

→ [Voltar ao README](Home/Projetos/MS2_projeto2/README.md)

## Formato de entrada

```json
{
  "nodes": [
    {"id": 1, "x": 0.0, "y": 0.0},
    {"id": 2, "x": 4.0, "y": 0.0}
  ],
  "elements": [
    {"id": 1, "node_i": 1, "node_j": 2, "E": 210e9, "I": 8.333333333333333e-6}
  ],
  "supports": [
    {"node": 1, "fix_v": true},
    {"node": 2, "fix_v": true}
  ],
  "nodal_loads": [
    {"node": 2, "fy": -1000.0, "mz": 0.0}
  ],
  "distributed_loads": [
    {"element": 1, "q_i": -1000.0, "q_j": -1000.0}
  ],
  "point_loads": [
    {"element": 1, "x": 2.0, "fy": -1000.0}
  ]
}
```

### Campos

| Campo | Obrigatório | Descrição |
|---|---|---|
| `nodes[].id` | sim | Inteiro contíguo começando em 1 |
| `nodes[].x`, `nodes[].y` | sim | Coordenadas em metros |
| `elements[].id` | sim | Inteiro contíguo começando em 1 |
| `elements[].node_i`, `node_j` | sim | IDs de nós existentes |
| `elements[].E` | sim | Módulo de elasticidade em Pa |
| `elements[].I` | sim | Momento de inércia em m⁴ |
| `supports[].node` | sim | Nó restringido |
| `supports[].fix_v` | não | Restringe o deslocamento vertical |
| `supports[].fix_theta` | não | Restringe o giro |
| `nodal_loads[].node` | sim | Nó de aplicação |
| `nodal_loads[].fy` | não | Força vertical em N (positiva para cima) |
| `nodal_loads[].mz` | não | Momento em N·m (positivo anti-horário) |
| `distributed_loads[].element` | sim | Elemento carregado |
| `distributed_loads[].q_i`, `q_j` | não | Carga linear em N/m (positiva para cima) |
| `point_loads[].element` | sim | Elemento carregado |
| `point_loads[].x` | sim | Posição local em m, com `0 < x < L` |
| `point_loads[].fy` | não | Força vertical em N |

Regras:

- IDs de nós e elementos devem ser contíguos e começar em 1.
- Cargas pontuais nas extremidades devem ser modeladas como `nodal_loads`.
- O elemento tem apenas DOFs `v` e `θ`; não há restrição axial explícita.

## Formato de saída

```json
{
  "displacements": [
    {"node": 1, "v": 0.0, "theta": -0.0015238095238095236},
    {"node": 2, "v": 0.0, "theta": 0.0015238095238095239}
  ],
  "reactions": [
    {"node": 1, "fy": 2000.0, "mz": 0.0},
    {"node": 2, "fy": 2000.0, "mz": 0.0}
  ],
  "elements": [
    {
      "id": 1,
      "end_forces": {"vi": 2000.0, "mi": 0.0, "vj": 2000.0, "mj": 0.0},
      "diagram": {
        "x": [0.0, 2.0, 4.0],
        "shear": [2000.0, 0.0, -2000.0],
        "moment": [0.0, 2000.0, 0.0],
        "deflection": [0.0, -0.0019047619047619043, 0.0]
      }
    }
  ],
  "maxima": {
    "deflection": 0.0019047619047619043,
    "shear": 2000.0,
    "moment": 2000.0
  }
}
```

Observações:

- `reactions` é emitido para todos os nós; nós sem restrição têm valores nulos.
- `diagram` contém `samples_per_element` amostras, incluindo as extremidades.
- `maxima` usa valores absolutos.
- A deformada é positiva para cima.

## CLI

```bash
# Demo embutida
./build/ms2_cli

# Ler JSON e imprimir no stdout
./build/ms2_cli -i examples/01-cantilever/input.json

# Salvar em arquivo
./build/ms2_cli -i examples/01-cantilever/input.json -o /tmp/out.json

# Controlar a resolução dos diagramas
./build/ms2_cli -i examples/01-cantilever/input.json --samples 2001
```

Opções:

| Opção | Descrição |
|---|---|
| `-i`, `--input` | Arquivo JSON de entrada |
| `-o`, `--output` | Arquivo JSON de saída |
| `-s`, `--samples` | Amostras por elemento (padrão 101) |
| `-h`, `--help` | Ajuda |

## API HTTP

Base: `http://127.0.0.1:8765`.

| Método | Rota | Descrição |
|---|---|---|
| `GET` | `/api/health` | Verifica se o backend está ativo |
| `GET` | `/api/examples` | Lista os exemplos disponíveis |
| `GET` | `/api/examples/<nome>` | Devolve o `input.json` do exemplo |
| `POST` | `/api/solve?samples=N` | Resolve o modelo enviado no corpo |

### Exemplos

```bash
curl http://127.0.0.1:8765/api/health

curl http://127.0.0.1:8765/api/examples

curl http://127.0.0.1:8765/api/examples/01-cantilever

curl -X POST \
  -H 'Content-Type: application/json' \
  --data @examples/02-simply-supported-udl/input.json \
  'http://127.0.0.1:8765/api/solve?samples=401'
```

Resposta de erro:

```json
{"error": "Point load x must be strictly inside the element; use a nodal load at its ends"}
```

Códigos:

| Código | Significado |
|---|---|
| 200 | Sucesso |
| 400 | JSON inválido, modelo inválido ou parâmetro inválido |
| 404 | Rota ou exemplo inexistente |
| 413 | Corpo maior que 5 MB |
| 500 | CLI ausente ou saída inválida |
| 504 | Timeout do solver |

## Formato Fortran

O PDF de referência usa um formato textual próprio. O adaptador para esse formato ainda não foi implementado; o contrato atual é JSON. A compatibilidade está no escopo do projeto e pode ser adicionada como um conversor de entrada/saída sem alterar o núcleo.
