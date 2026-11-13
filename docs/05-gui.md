---
type: content
status: active
updated: "2026-11-08"
tags: [projetos, pessoal]
---

# 05 — GUI

Interface desktop em Electron para editar modelos, resolver e visualizar resultados.

→ [Voltar ao README](Home/Projetos/MS2_projeto2/README.md)

## Executar

```bash
# Na raiz do projeto
./scripts/build.sh Debug

cd src/electron
npm install
npm start
```

O processo principal inicia o backend Python em `http://127.0.0.1:8765` e o encerra ao sair. A CLI `build/ms2_cli` precisa existir.

Para abrir o DevTools:

```bash
MS2_DEV=1 npm start
```

Em ambientes com Electron instalado no sistema:

```bash
cd src/electron
electron39 . --no-sandbox
```

## Layout

```text
┌──────────────────────────────────────────────────────────────┐
│ TopBar: MS2 Beam | Exemplo ▾ | Resolver | Status             │
├──────────────┬──────────────────────────────┬────────────────┤
│ Modelo JSON  │ Abas: Modelo / Deformada /   │ Máximas        │
│ Resumo       │       Cortante / Momento     │ Reações        │
│              │ Canvas SVG                   │ Deslocamentos  │
│              │ Legenda                      │ Extremidades   │
└──────────────┴──────────────────────────────┴────────────────┘
```

## Recursos

### Modelo

- Editor JSON com botões **Aplicar**, **Importar** e **Exportar**.
- Seletor de exemplos embutidos.
- Resumo com número de nós, elementos, apoios, cargas e vão total.

### Canvas

- **Modelo:** geometria, nós, apoios, cargas nodais, distribuídas e pontuais.
- **Deformada:** configuração original tracejada e deformada ampliada.
- **Cortante:** diagrama `V(x)` preenchido.
- **Momento:** diagrama `M(x)` preenchido.

A ampliação da deformada é visual, escolhida para ocupar cerca de 20% da altura do canvas. Os valores numéricos reais ficam nos painéis.

### Resultados

- **Máximas:** `|v|`, `|V|` e `|M|`.
- **Reações:** `fy` e `mz` por nó.
- **Deslocamentos:** `v` e `θ` por nó.
- **Esforços nas extremidades:** `vi`, `mi`, `vj`, `mj` por elemento.

## Fluxo de uso

1. Selecione um exemplo ou cole um modelo JSON.
2. Clique em **Aplicar**.
3. O renderer envia o modelo para o backend.
4. O backend executa a CLI e devolve o resultado.
5. Navegue pelas abas para inspecionar a resposta.

## Limitações e roadmap

Implementado no MVP:

- Edição por JSON.
- Visualização SVG.
- Quatro abas de resultado.
- Importação/exportação.
- Backend automático.

Roadmap de UX:

- Criar e arrastar nós/elementos no canvas.
- Pan, zoom e fit view.
- Inspector do elemento selecionado.
- Leitura de valores no hover.
- Painel de equações e explicação passo a passo.
- Autosave em `localStorage`.
- Exportação de imagens dos diagramas.

## Paleta

| Uso | Cor |
|---|---|
| Fundo | `#0B0E1A` |
| Painéis | `#111527` |
| Texto | `#E6EAF5` |
| Acento | `#5B8CFF` |
| Deformada | `#36D6B5` |
| Carga / cortante | `#FFB454` |
| Momento | `#A78BFA` |
