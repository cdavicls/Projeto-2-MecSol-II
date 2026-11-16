// Renderer entry point: owns the UI state, talks to the Python backend and
// draws the beam model/results as SVG. All numerical work happens in C++.
const API = "http://127.0.0.1:8765"; // Local backend started by main.js.
const SAMPLE_COUNT = 401; // Diagram samples requested per element.
const SVG_NS = "http://www.w3.org/2000/svg";

// Single source of truth for what is currently displayed in the renderer.
// `result` is replaced after every successful solve and cleared on edits/errors.
const state = {
  model: null,
  result: null,
  view: "model",
  examples: [],
  busy: false,
};

// Tiny DOM lookup helper to keep the rendering code compact.
const $ = (id) => document.getElementById(id);

function sleep(milliseconds) {
  // Promise wrapper used by the backend readiness poll below.
  return new Promise((resolve) => setTimeout(resolve, milliseconds));
}

function fmt(value, digits = 4) {
  // Human-friendly number formatting for the tables and SVG labels.
  // Very large/small values use scientific notation; invalid values use an em dash.
  if (value === null || value === undefined || !Number.isFinite(value)) {
    return "—";
  }
  if (value === 0) {
    return "0";
  }
  const absolute = Math.abs(value);
  if (absolute >= 1e5 || absolute < 1e-3) {
    return value.toExponential(digits);
  }
  return value.toLocaleString("pt-BR", { maximumFractionDigits: digits });
}

function setStatus(message, kind = "") {
  // `kind` is an optional CSS modifier: "", "ok" or "error".
  const element = $("status");
  element.textContent = message;
  element.className = `status ${kind}`.trim();
}

function setBusy(busy) {
  // Prevent overlapping solve/apply requests while the backend is working.
  state.busy = busy;
  $("solve-button").disabled = busy;
  $("apply-button").disabled = busy;
}

function showError(error) {
  // Log the original object for debugging and show a short message in the UI.
  console.error(error);
  const message = error && error.message ? error.message : String(error);
  setStatus(`Erro: ${message}`, "error");
}

async function waitForBackend() {
  // Electron may start before uvicorn/HTTPServer is listening, so poll
  // /api/health for up to ~10 seconds before loading examples.

  // 40 attempts x 250 ms = 10 s maximum wait.
  for (let attempt = 0; attempt < 40; attempt += 1) {
    try {
      const response = await fetch(`${API}/api/health`, { cache: "no-store" });
      if (response.ok) {
        return;
      }
    } catch (error) {
      // The backend may still be starting; keep polling.
    }
    await sleep(250);
  }
  throw new Error("backend não respondeu em http://127.0.0.1:8765");
}

async function loadExamples() {
  // Populate the example selector from the backend's example catalog and
  // automatically open the first entry so the app starts with a valid model.
  const response = await fetch(`${API}/api/examples`, { cache: "no-store" });
  if (!response.ok) {
    throw new Error("falha ao listar os exemplos");
  }
  const payload = await response.json();
  state.examples = payload.examples || [];

  const select = $("example-select");
  select.innerHTML = "";
  for (const example of state.examples) {
    const option = document.createElement("option");
    option.value = example.name;
    option.textContent = example.description
      ? `${example.name} — ${example.description}`
      : example.name;
    select.appendChild(option);
  }

  if (state.examples.length === 0) {
    setStatus("Nenhum exemplo", "error");
    return;
  }

  select.value = state.examples[0].name;
  await loadExample(state.examples[0].name);
}

async function loadExample(name) {
  // Fetch one example JSON, put it in the editor and solve it immediately.
  if (!name) {
    return;
  }
  setStatus("Carregando...");
  const response = await fetch(`${API}/api/examples/${encodeURIComponent(name)}`, {
    cache: "no-store",
  });
  if (!response.ok) {
    throw new Error(`falha ao carregar o exemplo ${name}`);
  }
  const model = await response.json();
  setModel(model);
  await solve();
}

function setModel(model) {
  // Replace the active model, clear stale results and refresh all panels.
  // The editor always shows pretty-printed JSON so it remains hand-editable.
  state.model = model;
  state.result = null;
  $("model-editor").value = JSON.stringify(model, null, 2);
  $("empty-state").hidden = true;
  render();
}

function applyModel() {
  // Parse the editor contents, validate the top-level JSON shape, then solve.
  try {
    const model = JSON.parse($("model-editor").value);
    if (!model || typeof model !== "object" || Array.isArray(model)) {
      throw new Error("o modelo deve ser um objeto JSON");
    }
    setModel(model);
    solve().catch(showError);
  } catch (error) {
    showError(error);
  }
}

async function solve() {
  // POST the current model to the backend. The backend returns the full
  // result payload (reactions, displacements, end forces and diagrams).
  if (!state.model || state.busy) {
    return;
  }

  setBusy(true);
  setStatus("Resolvendo...");
  try {
    const response = await fetch(`${API}/api/solve?samples=${SAMPLE_COUNT}`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(state.model),
    });
    const payload = await response.json();
    if (!response.ok) {
      // Backend errors are JSON: { "error": "..." }.
      throw new Error(payload.error || `falha na resolução (${response.status})`);
    }
    state.result = payload;
    render();
    setStatus("Resolvido", "ok");
  } catch (error) {
    // Keep the model visible but discard results that no longer match it.
    state.result = null;
    render();
    showError(error);
  } finally {
    setBusy(false);
  }
}

function render() {
  // Refresh every panel from the current state. Safe to call with or without
  // results; each renderer handles the empty case.
  renderSummary();
  renderMaxima();
  renderReactions();
  renderDisplacements();
  renderEndForces();
  renderCanvas();
  renderLegend();
}

function renderSummary() {
  // Model-level counts and total span shown above the canvas.
  const model = state.model;
  const element = $("summary");
  if (!model) {
    element.innerHTML = '<p class="muted">Sem modelo.</p>';
    return;
  }

  const nodes = model.nodes || [];
  const elements = model.elements || [];
  const supports = model.supports || [];
  const loadCount =
    (model.nodal_loads || []).length +
    (model.distributed_loads || []).length +
    (model.point_loads || []).length;
  const xs = nodes.map((node) => Number(node.x)).filter(Number.isFinite);
  const span = xs.length > 0 ? Math.max(...xs) - Math.min(...xs) : 0;

  element.innerHTML = `
    <div class="metric"><span>Nós</span><strong>${nodes.length}</strong></div>
    <div class="metric"><span>Elementos</span><strong>${elements.length}</strong></div>
    <div class="metric"><span>Apoios</span><strong>${supports.length}</strong></div>
    <div class="metric"><span>Cargas</span><strong>${loadCount}</strong></div>
    <div class="metric" style="grid-column: span 2"><span>Vão total</span><strong>${fmt(span)} m</strong></div>
  `;
}

function renderMaxima() {
  // Global peak values returned by the solver, independent of the active view.
  const element = $("maxima");
  if (!state.result) {
    element.innerHTML = '<p class="muted">Sem resultados.</p>';
    return;
  }
  const maxima = state.result.maxima || {};
  element.innerHTML = `
    <div class="metric"><span>|v| max</span><strong>${fmt(maxima.deflection)}</strong><small>m</small></div>
    <div class="metric"><span>|V| max</span><strong>${fmt(maxima.shear)}</strong><small>N</small></div>
    <div class="metric"><span>|M| max</span><strong>${fmt(maxima.moment)}</strong><small>N·m</small></div>
  `;
}

function tableHTML(headers, rows) {
  // Small helper shared by the reactions/displacements/end-force tables.
  const head = headers.map((header) => `<th>${header}</th>`).join("");
  const body = rows
    .map((row) => `<tr>${row.map((cell) => `<td>${cell}</td>`).join("")}</tr>`)
    .join("");
  return `<table><thead><tr>${head}</tr></thead><tbody>${body}</tbody></table>`;
}

function renderReactions() {
  // Support reactions: vertical force Fy and moment Mz per constrained node.
  const element = $("reactions");
  const reactions = state.result ? state.result.reactions || [] : [];
  if (reactions.length === 0) {
    element.innerHTML = '<p class="muted">Nenhuma reação de apoio.</p>';
    return;
  }
  element.innerHTML = tableHTML(
    ["Nó", "Fy (N)", "Mz (N·m)"],
    reactions.map((reaction) => [reaction.node, fmt(reaction.fy), fmt(reaction.mz)])
  );
}

function renderDisplacements() {
  // Nodal DOFs: transverse displacement v and rotation theta.
  const element = $("displacements");
  const displacements = state.result ? state.result.displacements || [] : [];
  if (displacements.length === 0) {
    element.innerHTML = '<p class="muted">Sem deslocamentos.</p>';
    return;
  }
  element.innerHTML = tableHTML(
    ["Nó", "v (m)", "θ (rad)"],
    displacements.map((item) => [item.node, fmt(item.v), fmt(item.theta)])
  );
}

function renderEndForces() {
  // Element-end internal forces in global sign convention: Vi, Mi, Vj, Mj.
  const element = $("end-forces");
  const elements = state.result ? state.result.elements || [] : [];
  if (elements.length === 0) {
    element.innerHTML = '<p class="muted">Sem esforços.</p>';
    return;
  }
  element.innerHTML = tableHTML(
    ["Elem.", "Vi (N)", "Mi (N·m)", "Vj (N)", "Mj (N·m)"],
    elements.map((item) => {
      const forces = item.end_forces || {};
      return [
        item.id,
        fmt(forces.vi),
        fmt(forces.mi),
        fmt(forces.vj),
        fmt(forces.mj),
      ];
    })
  );
}

function renderLegend() {
  // The legend follows the active tab so colors/symbols always match the canvas.
  const element = $("legend");
  if (state.view === "model") {
    element.innerHTML = `
      <span><i class="beam"></i>Elemento</span>
      <span><i class="node"></i>Nó</span>
      <span><i class="support-fixed"></i>Engaste</span>
      <span><i class="support-pinned"></i>Pino</span>
      <span><i class="support-roller"></i>Rolete</span>
      <span><i class="load"></i>Carga</span>
    `;
    return;
  }
  if (state.view === "deflection") {
    element.innerHTML = `
      <span><i class="original"></i>Configuração original</span>
      <span><i class="deflection"></i>Deformada (ampliada)</span>
    `;
    return;
  }
  if (state.view === "shear") {
    element.innerHTML = '<span><i class="shear"></i>Cortante V(x) — N</span>';
    return;
  }
  element.innerHTML = '<span><i class="moment"></i>Momento M(x) — N·m</span>';
}

function svgNode(name, attributes = {}) {
  // Create an SVG element in the correct namespace and apply attributes.
  const node = document.createElementNS(SVG_NS, name);
  for (const [key, value] of Object.entries(attributes)) {
    if (value !== undefined && value !== null) {
      node.setAttribute(key, String(value));
    }
  }
  return node;
}

function addLine(svg, x1, y1, x2, y2, className, attributes = {}) {
  // Line helper; extra attributes are used for load arrow markers.
  const line = svgNode("line", {
    x1,
    y1,
    x2,
    y2,
    class: className,
    ...attributes,
  });
  svg.appendChild(line);
  return line;
}

function addCircle(svg, cx, cy, radius, className, attributes = {}) {
  // Circle helper used for nodes and roller wheels.
  const circle = svgNode("circle", {
    cx,
    cy,
    r: radius,
    class: className,
    ...attributes,
  });
  svg.appendChild(circle);
  return circle;
}

function addText(svg, x, y, value, className, attributes = {}) {
  // Text helper; SVG text uses textContent instead of innerHTML.
  const text = svgNode("text", { x, y, class: className, ...attributes });
  text.textContent = value;
  svg.appendChild(text);
  return text;
}

function renderCanvas() {
  // Main drawing pipeline. Coordinates are converted from model space (meters,
  // y pointing up) to screen space (pixels, y pointing down) by `map`.
  const svg = $("canvas");
  svg.innerHTML = "";
  const model = state.model;
  if (!model) {
    return;
  }

  const nodes = model.nodes || [];
  if (nodes.length === 0) {
    return;
  }

  // Keep a stable drawing area even before the flex layout has settled.
  const rectangle = svg.getBoundingClientRect();
  const width = Math.max(rectangle.width, 640);
  const height = Math.max(rectangle.height, 420);
  svg.setAttribute("viewBox", `0 0 ${width} ${height}`);
  svg.setAttribute("preserveAspectRatio", "xMidYMid meet");

  // Reusable arrow head for nodal, distributed and point loads.
  const defs = svgNode("defs");
  const marker = svgNode("marker", {
    id: "arrow-load",
    viewBox: "0 0 10 10",
    refX: "9",
    refY: "5",
    markerWidth: "7",
    markerHeight: "7",
    orient: "auto-start-reverse",
  });
  marker.appendChild(svgNode("path", { d: "M 0 0 L 10 5 L 0 10 z", fill: "#FFB454" }));
  defs.appendChild(marker);
  svg.appendChild(defs);

  // Fast lookups by numeric id because JSON ids may arrive as strings.
  const nodeById = new Map(nodes.map((node) => [Number(node.id), node]));
  const elements = model.elements || [];
  const elementsById = new Map(elements.map((element) => [Number(element.id), element]));

  // Fit the whole model into the viewport, preserving aspect ratio.
  const xs = nodes.map((node) => Number(node.x) || 0);
  const ys = nodes.map((node) => Number(node.y) || 0);
  const minX = Math.min(...xs);
  const maxX = Math.max(...xs);
  const minY = Math.min(...ys);
  const maxY = Math.max(...ys);
  const spanX = Math.max(maxX - minX, 1e-9);
  const spanY = Math.max(maxY - minY, 1e-9);
  const padding = 78;
  const scaleX = (width - 2 * padding) / spanX;
  const scaleY = spanY > 1e-9 ? (height - 2 * padding) / spanY : scaleX;
  const scale = Math.min(scaleX, scaleY);
  const centerX = (minX + maxX) / 2;
  const centerY = (minY + maxY) / 2;
  const map = (x, y) => [
    width / 2 + (x - centerX) * scale,
    height / 2 - (y - centerY) * scale,
  ];

  // 1. Beam elements and their labels.
  for (const element of elements) {
    const nodeI = nodeById.get(Number(element.node_i));
    const nodeJ = nodeById.get(Number(element.node_j));
    if (!nodeI || !nodeJ) {
      continue;
    }
    const [x1, y1] = map(Number(nodeI.x) || 0, Number(nodeI.y) || 0);
    const [x2, y2] = map(Number(nodeJ.x) || 0, Number(nodeJ.y) || 0);
    addLine(svg, x1, y1, x2, y2, "beam");
    if (state.view === "model") {
      addText(svg, (x1 + x2) / 2, (y1 + y2) / 2 - 12, `E${element.id}`, "element-label");
    }
  }

  // 2. Loads in model view, or result diagrams in the other tabs.
  if (state.view === "model") {
    drawLoads(svg, model, nodeById, elementsById, map);
  } else if (state.result) {
    if (state.view === "deflection") {
      drawDeflection(svg, model, state.result, nodeById, map, height);
    }
    if (state.view === "shear" || state.view === "moment") {
      drawDiagram(svg, model, state.result, nodeById, map, state.view, height);
    }
  }

  // 3. Supports are drawn after the beam so their symbols stay visible.
  for (const support of model.supports || []) {
    const node = nodeById.get(Number(support.node));
    if (!node) {
      continue;
    }
    drawSupport(svg, node, support, map);
  }

  // 4. Nodes on top of everything.
  for (const node of nodes) {
    const [x, y] = map(Number(node.x) || 0, Number(node.y) || 0);
    addCircle(svg, x, y, 4.5, "node");
    addText(svg, x, y - 12, `N${node.id}`, "node-label");
  }

  addText(svg, width / 2, height - 20, `Vão total = ${fmt(maxX - minX)} m`, "span-label");
}

function supportKind(support) {
  // Resolve the visual kind from the explicit `type` field, accepting both
  // English names and the Portuguese aliases used in examples.
  // Legacy models without `type` fall back to `fix_theta`.
  const type = String(support.type || "").toLowerCase();
  if (type === "fixed" || type === "engaste") {
    return "fixed";
  }
  if (type === "roller" || type === "rolete") {
    return "roller";
  }
  if (type === "pinned" || type === "pin" || type === "pino") {
    return "pinned";
  }
  return support.fix_theta ? "fixed" : "pinned";
}

function drawGroundHatch(svg, x, y, halfWidth = 19, depth = 8) {
  // Diagonal hatching below a support to indicate fixed ground.
  for (let offset = -halfWidth; offset <= halfWidth; offset += 9) {
    addLine(svg, x + offset, y, x + offset - 7, y + depth, "support");
  }
}

function drawFixedSupport(svg, x, y) {
  // Built-in support: vertical wall plus hatched base (restrains v and theta).
  addLine(svg, x, y, x, y + 30, "support");
  addLine(svg, x - 18, y + 30, x + 18, y + 30, "support");
  drawGroundHatch(svg, x, y + 30, 18, 8);
}

function drawPinnedSupport(svg, x, y) {
  // Pinned support: triangle resting directly on the ground (restrains v only).
  const triangle = svgNode("path", {
    d: `M ${x} ${y} L ${x - 15} ${y + 27} L ${x + 15} ${y + 27} Z`,
    class: "support",
  });
  svg.appendChild(triangle);
  addLine(svg, x - 19, y + 27, x + 19, y + 27, "support");
  drawGroundHatch(svg, x, y + 27, 19, 8);
}

function drawRollerSupport(svg, x, y) {
  // Roller support: triangle on three wheels. Numerically identical to pinned
  // in this 2-DOF beam formulation; only the drawing differs.
  const baseY = y + 23;
  const triangle = svgNode("path", {
    d: `M ${x} ${y} L ${x - 15} ${baseY} L ${x + 15} ${baseY} Z`,
    class: "support",
  });
  svg.appendChild(triangle);
  for (const offset of [-9, 0, 9]) {
    addCircle(svg, x + offset, baseY + 5, 4, "support");
  }
  addLine(svg, x - 19, baseY + 9, x + 19, baseY + 9, "support");
  drawGroundHatch(svg, x, baseY + 9, 19, 7);
}

function drawSupport(svg, node, support, map) {
  // Convert the support node position and dispatch to the correct symbol.
  const [x, y] = map(Number(node.x) || 0, Number(node.y) || 0);
  const kind = supportKind(support);
  if (kind === "fixed") {
    drawFixedSupport(svg, x, y);
    return;
  }
  if (kind === "roller") {
    drawRollerSupport(svg, x, y);
    return;
  }
  drawPinnedSupport(svg, x, y);
}

function drawLoads(svg, model, nodeById, elementsById, map) {
  // Draw all three load types supported by the API: nodal, distributed and
  // point loads inside an element. Arrow direction follows the sign of Fy/q.
  for (const load of model.nodal_loads || []) {
    const node = nodeById.get(Number(load.node));
    if (!node) {
      continue;
    }
    const [x, y] = map(Number(node.x) || 0, Number(node.y) || 0);
    const fy = Number(load.fy) || 0;
    const mz = Number(load.mz) || 0;
    if (fy !== 0) {
      // Negative Fy points down; positive Fy points up.
      const length = 64;
      const startY = fy < 0 ? y - length : y + length;
      addLine(svg, x, startY, x, y, "load", { "marker-end": "url(#arrow-load)" });
      addText(svg, x + 8, (startY + y) / 2, `${fmt(fy)} N`, "load-label", {
        "text-anchor": "start",
      });
    }
    if (mz !== 0) {
      // Moments are shown as a text label at the node.
      addText(svg, x + 10, y - 22, `M=${fmt(mz)} N·m`, "load-label", {
        "text-anchor": "start",
      });
    }
  }

  const distributed = model.distributed_loads || [];
  // Largest |q| normalizes arrow lengths so small loads remain visible.
  const maxQ = Math.max(
    1e-9,
    ...distributed.flatMap((load) => [
      Math.abs(Number(load.q_i) || 0),
      Math.abs(Number(load.q_j) || 0),
    ])
  );

  for (const load of distributed) {
    // Distributed loads act along an element; draw ten arrows interpolating
    // linearly between q_i and q_j.
    const element = elementsById.get(Number(load.element));
    if (!element) {
      continue;
    }
    const nodeI = nodeById.get(Number(element.node_i));
    const nodeJ = nodeById.get(Number(element.node_j));
    if (!nodeI || !nodeJ) {
      continue;
    }
    const xi = Number(nodeI.x) || 0;
    const yi = Number(nodeI.y) || 0;
    const xj = Number(nodeJ.x) || 0;
    const yj = Number(nodeJ.y) || 0;
    const dx = xj - xi;
    const dy = yj - yi;
    const length = Math.hypot(dx, dy) || 1;
    const ux = dx / length;
    const uy = dy / length;
    const qi = Number(load.q_i) || 0;
    const qj = Number(load.q_j) || 0;

    for (let step = 1; step <= 9; step += 1) {
      const fraction = step / 10;
      const localX = fraction * length;
      const [x, y] = map(xi + ux * localX, yi + uy * localX);
      const q = qi + (qj - qi) * fraction;
      const magnitude = Math.abs(q);
      if (magnitude < 1e-9) {
        continue;
      }
      const arrowLength = 18 + 44 * (magnitude / maxQ);
      const startY = q < 0 ? y - arrowLength : y + arrowLength;
      addLine(svg, x, startY, x, y, "load", { "marker-end": "url(#arrow-load)" });
    }

    // Labels at both ends document the actual q values used by the solver.
    const [x1, y1] = map(xi, yi);
    const [x2, y2] = map(xj, yj);
    addText(svg, x1, y1 + (qi < 0 ? -68 : 76), `q=${fmt(qi)} N/m`, "load-label");
    addText(svg, x2, y2 + (qj < 0 ? -68 : 76), `q=${fmt(qj)} N/m`, "load-label");
  }

  for (const load of model.point_loads || []) {
    // Point loads are positioned by local coordinate x measured from node i.
    const element = elementsById.get(Number(load.element));
    if (!element) {
      continue;
    }
    const nodeI = nodeById.get(Number(element.node_i));
    const nodeJ = nodeById.get(Number(element.node_j));
    if (!nodeI || !nodeJ) {
      continue;
    }
    const xi = Number(nodeI.x) || 0;
    const yi = Number(nodeI.y) || 0;
    const xj = Number(nodeJ.x) || 0;
    const yj = Number(nodeJ.y) || 0;
    const dx = xj - xi;
    const dy = yj - yi;
    const length = Math.hypot(dx, dy) || 1;
    const localX = Number(load.x) || 0;
    const [x, y] = map(xi + (dx / length) * localX, yi + (dy / length) * localX);
    const fy = Number(load.fy) || 0;
    if (fy !== 0) {
      const arrowLength = 72;
      const startY = fy < 0 ? y - arrowLength : y + arrowLength;
      addLine(svg, x, startY, x, y, "load", { "marker-end": "url(#arrow-load)" });
      addText(svg, x + 8, (startY + y) / 2, `${fmt(fy)} N`, "load-label", {
        "text-anchor": "start",
      });
    }
  }
}

function drawDeflection(svg, model, result, nodeById, map, height) {
  // Overlay the deformed shape on the original beam. Displacements are
  // amplified to ~20% of the canvas height so small deflections are visible.
  const elements = model.elements || [];
  const resultById = new Map(
    (result.elements || []).map((element) => [Number(element.id), element])
  );

  let maxAbsolute = 0;
  for (const element of elements) {
    const data = resultById.get(Number(element.id));
    const values = data && data.diagram ? data.diagram.deflection || [] : [];
    for (const value of values) {
      maxAbsolute = Math.max(maxAbsolute, Math.abs(Number(value) || 0));
    }
  }
  const valueScale = maxAbsolute > 0 ? (height * 0.2) / maxAbsolute : 1;

  for (const element of elements) {
    const nodeI = nodeById.get(Number(element.node_i));
    const nodeJ = nodeById.get(Number(element.node_j));
    const data = resultById.get(Number(element.id));
    if (!nodeI || !nodeJ || !data || !data.diagram) {
      continue;
    }
    const xi = Number(nodeI.x) || 0;
    const yi = Number(nodeI.y) || 0;
    const xj = Number(nodeJ.x) || 0;
    const yj = Number(nodeJ.y) || 0;
    const dx = xj - xi;
    const dy = yj - yi;
    const length = Math.hypot(dx, dy) || 1;
    const ux = dx / length;
    const uy = dy / length;
    const xs = data.diagram.x || [];
    const values = data.diagram.deflection || [];
    if (xs.length === 0) {
      continue;
    }

    // Reference line for the undeformed element.
    const [x1, y1] = map(xi, yi);
    const [x2, y2] = map(xj, yj);
    addLine(svg, x1, y1, x2, y2, "beam-original");

    // Sample points along the element, offset vertically by v(x) * scale.
    const points = xs.map((localX, index) => {
      const [px, py] = map(xi + ux * Number(localX), yi + uy * Number(localX));
      return [px, py - (Number(values[index]) || 0) * valueScale];
    });
    const path = svgNode("path", {
      d: points.map(([x, y], index) => `${index === 0 ? "M" : "L"} ${x} ${y}`).join(" "),
      class: "diagram-deflection",
    });
    svg.appendChild(path);
  }
}

function drawDiagram(svg, model, result, nodeById, map, kind, height) {
  // Draw shear (V) or bending moment (M) diagrams. `kind` is "shear" or
  // "moment" and selects both the data array and the CSS classes.
  const elements = model.elements || [];
  const resultById = new Map(
    (result.elements || []).map((element) => [Number(element.id), element])
  );

  let maxAbsolute = 0;
  for (const element of elements) {
    const data = resultById.get(Number(element.id));
    const values = data && data.diagram ? data.diagram[kind] || [] : [];
    for (const value of values) {
      maxAbsolute = Math.max(maxAbsolute, Math.abs(Number(value) || 0));
    }
  }
  const valueScale = maxAbsolute > 0 ? (height * 0.24) / maxAbsolute : 1;
  const fillClass = kind === "shear" ? "diagram-shear-fill" : "diagram-moment-fill";
  const lineClass = kind === "shear" ? "diagram-shear" : "diagram-moment";

  for (const element of elements) {
    const nodeI = nodeById.get(Number(element.node_i));
    const nodeJ = nodeById.get(Number(element.node_j));
    const data = resultById.get(Number(element.id));
    if (!nodeI || !nodeJ || !data || !data.diagram) {
      continue;
    }
    const xi = Number(nodeI.x) || 0;
    const yi = Number(nodeI.y) || 0;
    const xj = Number(nodeJ.x) || 0;
    const yj = Number(nodeJ.y) || 0;
    const dx = xj - xi;
    const dy = yj - yi;
    const length = Math.hypot(dx, dy) || 1;
    const ux = dx / length;
    const uy = dy / length;
    const xs = data.diagram.x || [];
    const values = data.diagram[kind] || [];
    if (xs.length === 0) {
      continue;
    }

    // Baseline is the undeformed beam; the curve is offset by the sampled
    // internal force, and the area between them is filled for readability.
    const [baseX1, baseY1] = map(xi, yi);
    const [baseX2, baseY2] = map(xj, yj);
    const points = xs.map((localX, index) => {
      const [px, py] = map(xi + ux * Number(localX), yi + uy * Number(localX));
      return [px, py - (Number(values[index]) || 0) * valueScale];
    });

    const fillPath = svgNode("path", {
      d: [
        `M ${baseX1} ${baseY1}`,
        ...points.map(([x, y]) => `L ${x} ${y}`),
        `L ${baseX2} ${baseY2}`,
        "Z",
      ].join(" "),
      class: fillClass,
    });
    svg.appendChild(fillPath);

    const curvePath = svgNode("path", {
      d: points.map(([x, y], index) => `${index === 0 ? "M" : "L"} ${x} ${y}`).join(" "),
      class: lineClass,
    });
    svg.appendChild(curvePath);
    addLine(svg, baseX1, baseY1, baseX2, baseY2, "axis");
  }
}

function downloadJSON(filename, payload) {
  // Trigger a browser download for the current model without a backend call.
  const blob = new Blob([JSON.stringify(payload, null, 2)], {
    type: "application/json",
  });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = filename;
  link.click();
  URL.revokeObjectURL(url);
}

function bindEvents() {
  // Wire all buttons, file input, view tabs and the example selector once.
  $("solve-button").addEventListener("click", () => {
    solve().catch(showError);
  });

  $("apply-button").addEventListener("click", applyModel);

  $("import-button").addEventListener("click", () => {
    $("file-input").click();
  });

  $("file-input").addEventListener("change", async (event) => {
    // Import a local JSON model, validate by parsing, then solve it.
    const file = event.target.files && event.target.files[0];
    if (!file) {
      return;
    }
    try {
      const text = await file.text();
      const model = JSON.parse(text);
      setModel(model);
      await solve();
    } catch (error) {
      showError(error);
    } finally {
      event.target.value = "";
    }
  });

  $("export-button").addEventListener("click", () => {
    // Export only the model; results are reproducible by solving again.
    if (!state.model) {
      showError(new Error("não há modelo para exportar"));
      return;
    }
    downloadJSON("modelo-viga.json", state.model);
  });

  $("view-tabs").addEventListener("click", (event) => {
    // Tabs only change the visualization; the model/result stay untouched.
    const button = event.target.closest("button[data-view]");
    if (!button) {
      return;
    }
    state.view = button.dataset.view;
    for (const tab of $("view-tabs").querySelectorAll("button")) {
      tab.classList.toggle("active", tab === button);
    }
    renderCanvas();
    renderLegend();
  });

  $("example-select").addEventListener("change", (event) => {
    loadExample(event.target.value).catch(showError);
  });
}

async function init() {
  // Startup sequence: wire UI, wait for backend, load examples, solve first.
  bindEvents();
  setStatus("Conectando...");
  try {
    await waitForBackend();
    await loadExamples();
    setStatus("Pronto", "ok");
  } catch (error) {
    showError(error);
  }
}

document.addEventListener("DOMContentLoaded", init);
