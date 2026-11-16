---
type: content
status: active
updated: "2026-11-14"
tags: [projetos, pessoal]
---

# Relatório LaTeX — Projeto 2

Projeto LaTeX autocontido para o relatório do Projeto 2 de SET0184. O documento explica a teoria da linha elástica, a formulação do elemento finito de viga Euler-Bernoulli, o mapeamento entre teoria e código C++ e um exemplo de viga em balanço resolvido passo a passo.

→ [README do projeto](Home/Projetos/MS2_projeto2/README.md)

## Como importar no Overleaf

1. Baixe o arquivo `ms2-relatorio-overleaf.zip`.
2. No Overleaf, escolha **New Project → Upload Project**.
3. Selecione o ZIP.
4. Confirme que o arquivo principal é `main.tex` e que o compilador é **pdfLaTeX**.
5. Clique em **Recompile**.

## Estrutura

- `main.tex` — documento principal.
- `codigo/` — cópias dos fontes C++ e dos JSONs usados nos listings.
- `figuras/` — reservada para figuras externas; as figuras atuais são TikZ inline.

## Conteúdo do documento

- Teoria da linha elástica: hipóteses, cinemática, tensões, momento fletor, equação governante, condições de contorno, soluções analíticas, convenções de sinal, superposição e limites.
- Formulação FEM: graus de liberdade, funções de Hermite, matriz de rigidez elementar, cargas equivalentes, montagem global, condições de contorno, solver e pós-processamento.
- Implementação: responsabilidades das classes e trechos comentados de `Section`, `Node`, `Element`, `Model`, `Assembler`, `BoundaryConditionHandler`, `LinearSolver`, `BeamSolver`, `PostProcessor`, `InputParser`, `OutputWriter` e `main`.
- Exemplo passo a passo: viga em balanço com carga pontual na extremidade, do JSON de entrada até a comparação com a solução analítica.
- Segundo exemplo com carga distribuída, validação, conclusão e apêndices.

## Compilação local

```bash
pdflatex main.tex
pdflatex main.tex
```

O PDF é gerado em `main.pdf`. Os arquivos auxiliares (`main.aux`, `main.log`, `main.out`, `main.toc`) são artefatos de compilação e não fazem parte do ZIP de entrega.
