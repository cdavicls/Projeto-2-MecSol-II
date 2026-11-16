---
type: content
status: active
updated: "2026-11-08"
tags: [projetos, pessoal]
---

# 07 — Build e execução

Guia completo para compilar, testar e executar CLI, backend e GUI.

→ [Voltar ao README](Home/Projetos/MS2_projeto2/README.md)

## Requisitos

| Ferramenta | Versão mínima | Uso |
|---|---|---|
| CMake | 3.20 | Build C++ |
| Compilador C++ | C++20 | Núcleo e CLI |
| Python | 3.9 | Backend e validação |
| Node.js | 18 | Electron |
| npm | 9 | Dependências do Electron |

O projeto foi testado com GCC 16.2.1, Python 3.14, Node 26 e Electron 39.

## Compilar

```bash
cd Home/Projetos/MS2_projeto2
chmod +x scripts/build.sh
./scripts/build.sh Debug
```

O script:

1. Configura o CMake em `build/`.
2. Compila `ms2_core`, `ms2_cli` e `ms2_tests`.
3. Executa o CTest.

Build manual:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## CLI

```bash
# Demo embutida
./build/ms2_cli

# Resolver um exemplo
./build/ms2_cli -i examples/example-1/input.json

# Salvar resultado
./build/ms2_cli -i examples/example-1/input.json -o /tmp/out.json

# Ajuda
./build/ms2_cli --help
```

## Testes

```bash
ctest --test-dir build --output-on-failure
```

Suítes:

- `ms2_tests`: 18 casos Catch2.
- `ms2_backend`: 6 casos de API Python.

Validação numérica:

```bash
./scripts/validate_examples.py
```

## Backend Python

```bash
python3 src/python/backend/server.py --port 8765
```

Opções:

| Opção | Padrão | Descrição |
|---|---|---|
| `--host` | `127.0.0.1` | Interface de escuta |
| `--port` | `8765` | Porta HTTP |
| `--cli` | `build/ms2_cli` | Caminho da CLI |
| `--examples` | `examples/` | Diretório de exemplos |

Variáveis de ambiente equivalentes: `MS2_CLI` e `MS2_EXAMPLES`.

Teste rápido:

```bash
curl http://127.0.0.1:8765/api/health
```

## GUI Electron

```bash
cd src/electron
npm install
npm start
```

O processo principal:

1. Inicia o backend Python.
2. Abre a janela do Electron.
3. Encerra o backend ao sair.

DevTools:

```bash
MS2_DEV=1 npm start
```

Electron do sistema:

```bash
cd src/electron
electron39 . --no-sandbox
```

## Teste de fumaça da GUI

O processo principal aceita variáveis para captura automatizada:

```bash
cd src/electron
MS2_SCREENSHOT=/tmp/ms2.png \
MS2_TEST_VIEW=moment \
MS2_EXIT_AFTER_SCREENSHOT=1 \
electron39 . --no-sandbox
```

`MS2_TEST_VIEW` aceita `model`, `deflection`, `shear` ou `moment`.

## Problemas comuns

### `Catch::Approx` não compila

Use `Approx(...)` sem qualificação. Em Catch2 v2, `Catch::Approx` resolve para o matcher de vetor `Catch::Matchers::Approx`.

### Backend não inicia

- Verifique se `python3` está no `PATH`.
- Verifique se a porta `8765` está livre.
- Confirme que `build/ms2_cli` existe.
- Rode o backend manualmente para ver o erro.

### `Solver CLI not found`

Compile a CLI:

```bash
./scripts/build.sh Debug
```

Ou informe o caminho:

```bash
python3 src/python/backend/server.py --cli /caminho/para/ms2_cli
```

### Electron não abre como root

Use `--no-sandbox`:

```bash
electron39 . --no-sandbox
```

### `npm install` falha

- Confirme a conexão com a internet.
- Verifique a versão do Node (`node --version`).
- Apague `node_modules/` e tente novamente.

### Resultados estranhos

- Confira as convenções de sinal em `docs/01-teoria.md`.
- Verifique se cargas pontuais estão dentro do elemento.
- Confirme que os IDs são contíguos e começam em 1.
- Aumente `--samples` para melhorar a leitura dos máximos.

## Limpeza

```bash
rm -rf build
rm -rf src/electron/node_modules
find . -name '__pycache__' -type d -prune -exec rm -rf {} +
```
