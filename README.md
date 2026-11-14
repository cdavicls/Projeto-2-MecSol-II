---
type: folder
status: active
updated: "2026-11-08"
tags: [projetos, pessoal, folder-index]
---

# MS2_projeto2

Projeto 2 da disciplina SET0184 — Mecânica dos Sólidos II (EESC-USP): implementação do elemento de viga da Eq. (32.17) com núcleo matemático em **C++ orientado a objetos**, backend em **Python** e GUI desktop em **Electron**, inspirada na experiência de uso do [StatIQ](https://www.statiqsolver.com/app).

**Documentação:** português (pt-BR) nos arquivos `.md`.
**Código:** inglês (identificadores, comentários e nomes de arquivos).
**UI:** português (pt-BR).

## Status

Fases 0–3 e documentação técnica (`docs/01`–`07`) concluídas (MVP): núcleo C++, CLI com JSON, backend Python, GUI Electron e quatro exemplos validados.
Próxima etapa: relatório LaTeX, slides, QA e ensaio.
Apresentação oral: **17/11/2026**.

## Objetivo

Implementar e validar um solver de vigas por elementos finitos (Euler-Bernoulli) capaz de analisar vigas com discretizações e condições de contorno diversas, submetidas simultaneamente a cargas concentradas e distribuídas, apresentando deslocamentos, giros, reações e esforços solicitantes nos nós.

## Requisitos do enunciado

- Implementar o elemento de viga discutido em sala (Eq. 32.17).
- Aceitar discretizações e condições de contorno diversas.
- Suportar ações concentradas e distribuídas ao longo do comprimento.
- Apresentar deslocamentos, giros e esforços solicitantes nos nós da malha.
- Comparar os resultados com respostas de referência em pelo menos dois exemplos.
- Apresentar oralmente em 17/11/2026.

## Escopo adicional

- Núcleo matemático em C++ com orientação a objetos.
- GUI desktop com Electron + Python.
- Documentação robusta da tradução teoria → matemática → código.
- Diagramas de esforço cortante, momento fletor e deformada.
- Carga pontual no interior do elemento (além das cargas nodais e distribuídas).
- Compatibilidade com o formato de entrada do programa Fortran do PDF.
- Testes automatizados (CTest).
- Relatório em LaTeX e slides para a apresentação oral.

## Decisões

| Tema | Decisão |
|---|---|
| Idioma da documentação | pt-BR nos arquivos `.md` |
| Idioma do código | inglês (identificadores, comentários e arquivos) |
| Idioma da UI | pt-BR |
| Arquitetura | Electron (UI) + Python stdlib (API local) + CLI C++ |
| Renderer | vanilla JS + SVG |
| Dependências | `npm install electron`; headers vendorizados (`nlohmann/json`, `Catch2`) |
| Entrada/saída | JSON principal + adaptador do formato Fortran |
| Solver | implementação própria (LU com pivoteamento parcial / LDLT) |
| Referência visual | StatIQ (`statiqsolver.com/app`) — apenas UX, sem copiar código ou assets |
| Escopo | completo (diagramas, carga pontual, relatório, input Fortran, testes) |

## Arquitetura

```text
Electron (UI desktop)
        │ HTTP/JSON
        ▼
Python backend (stdlib http.server)
        │ subprocess + JSON
        ▼
C++ core (libms2) + CLI (ms2_cli)
        │
        └── examples/, tests/, validação
```

- O **C++** concentra toda a matemática.
- O **Python** apenas orquestra e serve a API local.
- O **Electron** é a casca desktop e a interface.
- O **JSON** é o contrato único entre CLI, API, testes e GUI.

## Estrutura de pastas

Estrutura atual do projeto.

```text
MS2_projeto2/
├── README.md
├── CMakeLists.txt
├── .gitignore
├── Assets/
│   ├── Projeto 2.pdf
│   └── Teoria + Codigos.pdf
├── docs/
│   ├── 01-teoria.md
│   ├── 02-matematica-para-codigo.md
│   ├── 03-arquitetura.md
│   ├── 04-api-e-formatos.md
│   ├── 05-gui.md
│   ├── 06-exemplos-validacao.md
│   ├── 07-build-e-execucao.md
│   ├── relatorio/
│   │   └── figuras/
│   └── apresentacao/
├── src/
│   ├── cpp/
│   │   ├── include/ms2/
│   │   └── src/
│   ├── python/
│   │   └── backend/
│   │       └── server.py
│   └── electron/
│       ├── main.js
│       ├── preload.js
│       ├── package.json
│       └── renderer/
│           ├── index.html
│           ├── styles.css
│           ├── app.js
│           ├── components/
│           └── assets/
├── tests/
│   ├── cpp/
│   ├── python/
│   └── data/
├── examples/
│   ├── 01-cantilever/
│   ├── 02-simply-supported-udl/
│   ├── 03-simply-supported-linear/
│   └── 04-continuous-beam/
├── scripts/
├── third_party/
│   ├── nlohmann/
│   └── catch2/
├── Notas/
└── Tarefas/
```

## Núcleo matemático

### Elemento de viga

DOFs por nó: `[v, θz]` (deslocamento vertical e giro).
Vetor local: `q_e = [v_i, θ_i, v_j, θ_j]`.

Matriz de rigidez local (Eq. 32.17):

```text
        EI   ⎡  12    6L   -12    6L ⎤
K_e = ─────  ⎢  6L   4L²   -6L   2L² ⎥
       L³    ⎢ -12   -6L    12   -6L ⎥
             ⎣  6L   2L²   -6L   4L² ⎦
```

### Carga distribuída linear

Para `q(ξ) = q_i(1-ξ) + q_j ξ`:

```text
f_eq = [ (7q_i + 3q_j)L/20,
         (q_i/20 + q_j/30)L²,
         (3q_i + 7q_j)L/20,
        -(q_i/30 + q_j/20)L² ]
```

### Montagem e solução

- Matriz global montada por conectividade dos elementos.
- Condições de contorno por partição (ou zero linha/coluna, como no Fortran).
- Solver próprio (LU com pivoteamento parcial ou LDLT).
- Reações: `R = K·U − F`.
- Esforços de extremidade: `f_e = K_e·q_e − f_eq`.
- Diagramas: `V(x) = V_i + ∫q dx`, `M(x) = −M_i + ∫V dx`, deformada por Hermite.

### Classes implementadas (C++)

`Node`, `Section`, `Element`, `Model`, `NodalLoad`, `DistributedLoad`, `PointLoad`, `LoadCase`, `Assembler`, `BoundaryCondition`, `LinearSolver`, `PostProcessor`, `InputParser`, `OutputWriter`, `BeamSolver`.

## GUI inspirada no StatIQ

Implementação atual (MVP):

- **Editor JSON:** aplicar, importar e exportar o modelo.
- **Exemplos embutidos:** carregar os quatro casos de validação.
- **Canvas SVG:** geometria, nós, apoios, cargas nodais/distribuídas/pontuais, deformada ampliada, cortante e momento.
- **Abas:** modelo, deformada, cortante e momento.
- **Resultados:** máximas, reações, deslocamentos/giros e esforços de extremidade.
- **Backend automático:** o processo principal do Electron inicia o servidor Python e o encerra ao sair.

Ainda no roadmap de UX: edição visual por arraste, pan/zoom, leitura no hover, painel de equações, explicação passo a passo e autosave.

Paleta: fundo `#0B0E1A`, texto `#E6EAF5`, acento `#5B8CFF`; fontes Inter + JetBrains Mono.

## Exemplos de validação

| # | Exemplo | Referência analítica |
|---|---|---|
| 1 | Cantilever com carga pontual na ponta | `v_tip = −PL³/(3EI)`, `θ_tip = −PL²/(2EI)` |
| 2 | Simplesmente apoiada com UDL | `v_mid = −5qL⁴/(384EI)`, `M_mid = qL²/8` |
| 3 | Simplesmente apoiada com carga linear (exemplo do PDF) | `R_A = q₀L/6`, `R_B = q₀L/3`, `M_max = q₀L²/(9√3)` em `x = L/√3` |
| 4 | Viga contínua de 2 vãos com UDL | reações `3qL/8`, `10qL/8`, `3qL/8` |

Cada exemplo tem `input.json`, `expected.json` e `output.json`; a validação é feita por `scripts/validate_examples.py`.

## Documentação

- → [01 — Teoria](Home/Projetos/MS2_projeto2/docs/01-teoria.md) — linha elástica, Eq. 32.17, funções de forma, cargas equivalentes e convenções.
- → [02 — Matemática para código](Home/Projetos/MS2_projeto2/docs/02-matematica-para-codigo.md) — mapa equação → classe/método com referências `arquivo:linha`.
- → [03 — Arquitetura](Home/Projetos/MS2_projeto2/docs/03-arquitetura.md) — camadas, fluxo e decisões.
- → [04 — API e formatos](Home/Projetos/MS2_projeto2/docs/04-api-e-formatos.md) — schema JSON, CLI e API HTTP.
- → [05 — GUI](Home/Projetos/MS2_projeto2/docs/05-gui.md) — uso, recursos e roadmap.
- → [06 — Exemplos e validação](Home/Projetos/MS2_projeto2/docs/06-exemplos-validacao.md) — fórmulas, resultados e erros.
- → [07 — Build e execução](Home/Projetos/MS2_projeto2/docs/07-build-e-execucao.md) — build, testes, backend, Electron e troubleshooting.
- `docs/relatorio/` — relatório LaTeX em pt-BR (a produzir).
- `docs/apresentacao/` — slides da oral (a produzir).

## Cronograma

| Período | Entrega |
|---|---|
| 05–06/11 | Scaffold, C++ core (elemento + montagem) e teste cantilever |
| 07–08/11 | Cargas distribuídas, CCs, solver, pós-processamento e CLI |
| 09–10/11 | 4 exemplos de validação + testes automatizados |
| 11/11 | Backend Python + API + integração |
| 12–14/11 | GUI Electron inspirada no StatIQ |
| 15–16/11 | Documentação, relatório/slides, QA e ensaio |
| 17/11 | Apresentação oral |

## Riscos e limitações

- **Convenção de sinais** é o maior risco; será derivada e travada pelos exemplos.
- O Fortran rotula “Área” mas usa **I** (momento de inércia) no produto `EI`; o projeto padroniza `E` e `I`.
- O elemento tem apenas DOFs `v` e `θ` (sem axial/torção); assume-se viga horizontal.
- Hipóteses de Euler-Bernoulli: pequenas deformações, seções planas e sem deformação por cisalhamento.
- Electron exige `npm install`; o backend usa stdlib para evitar atrito com Python 3.14.
- StatIQ resolve equilíbrio estático; aqui ele é referência apenas de UX/visual.
- Não copiar código, assets ou identidade visual do StatIQ.

## Fase 0 (concluída)

- [x] Copiar os PDFs para `Assets/`.
- [x] Criar a estrutura de pastas.
- [x] Criar `CMakeLists.txt` e `.gitignore`.
- [x] Implementar o esqueleto do núcleo C++ (`Node`, `Section`, `Element`, `Model`).
- [x] Implementar a matriz de rigidez local e o primeiro teste (cantilever).
- [x] Configurar CTest.
- [x] Atualizar o índice `Home/Projetos/Projetos.md`.

### Build e testes

```bash
chmod +x scripts/build.sh
./scripts/build.sh Debug
./build/ms2_cli
```

O script configura o CMake, compila `ms2_core`, `ms2_cli` e `ms2_tests`, e executa o CTest.

## Fase 1 (concluída)

- [x] Cargas distribuídas lineares (`q_i`, `q_j`) e cargas pontuais no interior do elemento.
- [x] Vetores de carga consistente via funções de forma de Hermite.
- [x] Reações de apoio e esforços de extremidade por elemento.
- [x] Diagramas de cortante, momento e deformada (integração exata por elemento).
- [x] Entrada/saída JSON na CLI.
- [x] Quatro exemplos de validação e script `scripts/validate_examples.py`.

### Uso da CLI

```bash
# Demo embutida (cantilever)
./build/ms2_cli

# Resolver um exemplo e imprimir o JSON
./build/ms2_cli -i examples/02-simply-supported-udl/input.json

# Salvar o resultado e aumentar a resolução dos diagramas
./build/ms2_cli -i examples/03-simply-supported-linear/input.json \
  -o examples/03-simply-supported-linear/output.json --samples 2001

# Validar todos os exemplos contra as respostas analíticas
./scripts/validate_examples.py
```

### Formato JSON

Entrada:

```json
{
  "nodes": [{"id": 1, "x": 0.0, "y": 0.0}],
  "elements": [{"id": 1, "node_i": 1, "node_j": 2, "E": 210e9, "I": 8.33e-6}],
  "supports": [{"node": 1, "fix_v": true, "fix_theta": true}],
  "nodal_loads": [{"node": 2, "fy": -1000.0, "mz": 0.0}],
  "distributed_loads": [{"element": 1, "q_i": -1000.0, "q_j": -1000.0}],
  "point_loads": [{"element": 1, "x": 2.0, "fy": -1000.0}]
}
```

Saída: `displacements`, `reactions`, `elements` (esforços de extremidade e diagramas `x`, `shear`, `moment`, `deflection`) e `maxima`.

Convenções: `v` positivo para cima, `θ` anti-horário, `q` positivo para cima e momento fletor positivo quando traciona a fibra inferior (sagging).

## Fase 2 (concluída)

- [x] Backend Python com `http.server` (somente stdlib).
- [x] `GET /api/health`, `GET /api/examples`, `GET /api/examples/<nome>`.
- [x] `POST /api/solve?samples=N` com o modelo JSON no corpo.
- [x] Execução do `ms2_cli` via `subprocess` e arquivos temporários.
- [x] CORS para o renderer do Electron e tratamento de erros HTTP.
- [x] Testes Python com `unittest` integrados ao CTest (`ms2_backend`).

Executar o backend isoladamente:

```bash
python3 src/python/backend/server.py --port 8765
```

## Fase 3 (concluída — MVP)

- [x] Electron com `main.js`, `preload.js` e renderer vanilla JS + SVG.
- [x] Inicialização automática do backend Python pelo processo principal.
- [x] Editor JSON, importação/exportação e seletor de exemplos.
- [x] Canvas com modelo, deformada, cortante e momento.
- [x] Painéis de máximas, reações, deslocamentos e esforços de extremidade.
- [x] Teste de fumaça com captura de tela das quatro abas.

### Executar a GUI

```bash
# Pré-requisito: CLI compilada
./scripts/build.sh Debug

cd src/electron
npm install        # apenas na primeira vez
npm start
```

O Electron inicia o backend em `http://127.0.0.1:8765` automaticamente. Para abrir o DevTools:

```bash
MS2_DEV=1 npm start
```

Se o Electron estiver instalado no sistema (por exemplo, `electron39`), também é possível executar:

```bash
cd src/electron
electron39 . --no-sandbox
```

## Referências

- → [Projeto 2 (PDF)](<Home/Projetos/MS2_projeto2/Assets/Projeto 2.pdf>)
- → [Teoria + Códigos (PDF)](<Home/Projetos/MS2_projeto2/Assets/Teoria + Codigos.pdf>)
- → [Projeto 1 — relatório e código](Home/Faculdade/MecSol-2/MecSol-2.md)
- [StatIQ — referência de UX](https://www.statiqsolver.com/app)
