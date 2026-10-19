R"ZUCCHINI_REPORT(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8"><meta name="generator" content="Zucchini"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Zucchini · Business requirements</title>
<style>
:root{color-scheme:light;--paper:#f7f8fa;--ink:#202735;--muted:#657084;--line:#e1e5eb;--green:#136747;--red:#b53242;--amber:#946010;--purple:#6653b3;--strictness-ink:#202735;--strictness-exact:#cdebdc;--strictness-one-of:#d5e3f7;--strictness-bounded:#e4dcfa;--strictness-open-ended:#f8e5c4;--strictness-unspecified:#e5e8ed}*{box-sizing:border-box}body{margin:0;background:var(--paper);color:var(--ink);font:15px/1.55 system-ui,sans-serif}main{max-width:1320px;margin:auto;padding:36px 32px 72px}header{display:flex;justify-content:space-between;gap:20px;align-items:flex-start}.brand{font-size:12px;font-weight:750;letter-spacing:.13em;text-transform:uppercase;color:var(--green)}h1{font-size:34px;letter-spacing:-.04em;margin:8px 0}p{margin:6px 0;color:var(--muted)}.live{border:1px solid var(--line);border-radius:20px;padding:7px 12px;font-size:12px;background:white;white-space:nowrap}.live:before{content:'●';color:var(--green);margin-right:7px}.metrics{display:grid;grid-template-columns:repeat(auto-fit,minmax(170px,1fr));gap:14px;margin:28px 0}.metric{background:white;border:1px solid var(--line);padding:18px 20px;border-radius:12px}.metric strong{font-size:30px;font-weight:650;display:block;line-height:1.25}.metric span{color:var(--muted);font-size:13px}.toolbar{display:flex;gap:16px;align-items:center;justify-content:space-between;margin:22px 0}.tabs{display:flex;gap:5px}button{font:inherit;border:1px solid transparent;border-radius:7px;background:transparent;color:var(--muted);padding:8px 13px;cursor:pointer}button.active{background:white;border-color:var(--line);color:var(--ink);box-shadow:0 1px 3px #182b4110}input{font:inherit;background:white;border:1px solid var(--line);border-radius:7px;padding:9px 12px;width:280px}.card{background:white;border:1px solid var(--line);border-radius:12px;margin:18px 0;overflow:hidden}.cardhead{padding:19px 22px;display:flex;justify-content:space-between;gap:18px;align-items:flex-start}.cardhead h2{font-size:18px;margin:0}.cardhead p{font-size:14px}.pill{display:inline-block;border-radius:20px;padding:3px 9px;background:#eef1f5;color:var(--muted);font-size:12px;font-weight:650;white-space:nowrap}.passed{color:var(--green);background:#e8f5ee}.failed,.rejected{color:var(--red);background:#fcecef}.uncovered,.stale,.incomplete{color:var(--amber);background:#fff4dc}.covered{color:var(--purple);background:#f0edfc}.evidence{width:100%;border-collapse:collapse}td,th{text-align:left;padding:14px 22px;border-top:1px solid var(--line);vertical-align:top}th{font-size:11px;text-transform:uppercase;letter-spacing:.08em;color:var(--muted);background:#fafbfc}td:first-child{width:38%}td p{font-size:13px;margin:3px 0}.dim{font-size:12px;color:var(--muted)}details{margin-top:8px}summary{color:var(--purple);cursor:pointer;font-size:13px}.witness{border-left:2px solid var(--line);margin:13px 0;padding-left:12px}.source{overflow-wrap:anywhere;font:12px/1.5 ui-monospace,monospace;color:var(--muted)}pre{white-space:pre-wrap;overflow-wrap:anywhere;background:#f7f8fa;border-radius:6px;padding:12px;font:12px/1.6 ui-monospace,monospace}.notice{padding:17px 22px;border-top:1px solid var(--line);color:var(--muted)}.empty{padding:40px;border:1px dashed var(--line);border-radius:12px;text-align:center;color:var(--muted)}footer{font-size:12px;color:var(--muted);margin-top:24px}.progress{height:5px;background:#edf0f4}.progress i{display:block;background:var(--green);height:100%}@media(max-width:800px){main{padding:24px 16px}.metrics{grid-template-columns:repeat(2,1fr)}.toolbar,header{flex-wrap:wrap}td,th{padding:12px}input{width:100%}h1{font-size:28px}}@media(prefers-color-scheme:dark){:root{color-scheme:dark;--paper:#171b23;--ink:#e6eaf1;--muted:#a5afbf;--line:#343d4c;--green:#67cfa4;--red:#ffa1ae;--amber:#e9c178;--purple:#b1a2ff;--strictness-ink:#e6eaf1;--strictness-exact:#1f4b3a;--strictness-one-of:#233a5c;--strictness-bounded:#3a3062;--strictness-open-ended:#4d3a1b;--strictness-unspecified:#3a4352}.metric,.card,input,button.active,.live{background:#222833}th,pre{background:#1b202a}.passed{background:#193e32}.failed,.rejected{background:#4c2730}.uncovered,.stale,.incomplete{background:#423620}.cell.ignored{background:rgba(235,235,235,.22)}.covered{background:#332c4a}.progress{background:#343d4c}}
.scenario-card,.rule-card{overflow:visible}.witness .scenario-source{margin:12px 0}.source-line.selected-observation{border-color:var(--purple);background:color-mix(in srgb,var(--purple) 15%,transparent)}.observation.selected-observation{border-left:3px solid var(--purple);padding-left:10px}.scenario-source{margin:0 22px 22px;font:13px/1.7 ui-monospace,monospace}.source-line{position:relative;display:flex;gap:14px;padding:2px 10px;border-left:3px solid transparent;white-space:pre-wrap;overflow-wrap:anywhere}.line-number{color:var(--muted);min-width:3em;text-align:right;user-select:none}.line-text{flex:1}.source-line.coverage{border-color:var(--green);background:color-mix(in srgb,var(--green) 9%,transparent)}.source-line.violation{border-color:var(--red);background:color-mix(in srgb,var(--red) 9%,transparent)}.source-line.unjustified{border-color:var(--amber);background:color-mix(in srgb,var(--amber) 9%,transparent)}.cell{border-radius:3px}.cell.coverage{background:color-mix(in srgb,var(--green) 22%,transparent)}.cell.violation{background:color-mix(in srgb,var(--red) 22%,transparent)}.cell.unjustified{background:color-mix(in srgb,var(--amber) 30%,transparent)}.cell.ignored{background:#e4e5e7}.observations{display:none;position:absolute;z-index:5;left:4em;right:1em;top:100%;white-space:normal;background:var(--paper);border:1px solid var(--line);border-radius:8px;padding:14px;box-shadow:0 8px 24px #0003;font:13px/1.5 system-ui,sans-serif}.source-line:hover .observations,.source-line:focus .observations,.source-line:focus-within .observations{display:block}.source-line:focus{outline:2px solid var(--purple)}.observation+.observation{border-top:1px solid var(--line);margin-top:10px;padding-top:10px}
.strictness{display:inline-block;margin-left:7px;border-radius:5px;padding:1px 7px;font-size:11px;font-weight:650;line-height:1.6;white-space:nowrap;vertical-align:1px;color:var(--strictness-ink)}.strictness.exact{background:var(--strictness-exact)}.strictness.one-of{background:var(--strictness-one-of)}.strictness.bounded{background:var(--strictness-bounded)}.strictness.open-ended{background:var(--strictness-open-ended)}.strictness.unspecified{background:var(--strictness-unspecified)}.strictness-summary{display:flex;flex-wrap:wrap;gap:6px 14px;margin-top:8px;font-size:13px;color:var(--muted)}.strictness-summary .strictness{margin-left:0;margin-right:5px}.feature-group{margin:22px 0}.feature-group>h3{margin:0;font-size:17px}.feature-group>.source{margin-bottom:6px}.rule-group{margin:10px 0 0 14px;padding-left:14px;border-left:2px solid var(--line)}.rule-group>h4{margin:12px 0 0;font-size:14px;color:var(--muted)}.intro{margin:8px 0 18px}.intro h2{margin:0 0 4px;font-size:18px}a{color:var(--purple)}
</style>
</head>
<body data-server-no-reload><main>
<header><div><div class="brand">Zucchini / Rule coverage</div><h1 id="title">Business requirements</h1><p id="subtitle">Waiting for discovery data…</p></div><span class="live" id="live">Updates automatically</span></header>
<p><label>Load CTest results <input id="ctest-results" type="file" accept=".xml"></label> <span id="import-status" class="dim">Run ctest --output-junit results.xml, then select the file.</span></p>
<section id="diagnostics"></section><div class="metrics" id="metrics"></div>
<div class="toolbar"><nav class="tabs" aria-label="Report views"><button class="active" data-view="requirements">Requirements</button><button data-view="scenarios">Scenarios</button><button data-view="violations">Violations</button><button data-view="statistics">Statistics</button></nav><input id="search" type="search" aria-label="Search report" placeholder="Search rules, requirements, scenarios…"></div>

<section id="content" aria-live="polite"></section>
<footer>Coverage is evidence that a rule observed its requirement being tested. Assertions not tied to a requirement are outcome steps or data table cells that no business rule claims as coverage; Statistics summarizes them. Execution results are shown separately. Missing coverage is derived from declarations; it is not automatically a violation. Execution status comes from the selected CTest JUnit file. Import results from the same build; JUnit cannot verify the discovery identity. Reload the file after each test run.</footer>
</main>
<script>

'use strict';
const state = {
  report: null,
  results: new Map(),
  view: 'requirements',
  snapshot: ''
};
const node = (tag, cls, text) => {
  const n = document.createElement(tag);
  if (cls) n.className = cls;
  if (text !== undefined) n.textContent = text;
  return n
};
const identity = (fixture, name) => JSON.stringify([fixture, name]);
window.zucchiniRules = report => {
  state.report = report
};
window.zucchiniResult = result => {
  state.results.set(identity(result.fixture, result.test_name), result)
};
const resultFor = s => state.results.get(identity(state.report.fixture, s.test_name));
function status(s) {
  if (!s.accepted) return 'rejected';
  const r = resultFor(s);
  if (!r || r.status === 'not_run') return 'not_run';
  if (r.discovery_id !== state.report.discovery_id) return 'stale';
  return r.status === 'started' ? 'incomplete' : r.status
}
const labels = {
  passed: 'Passed',
  failed: 'Failed',
  skipped: 'Skipped',
  not_run: 'No imported result',
  stale: 'Previous discovery',
  incomplete: 'No final result',
  rejected: 'Rejected at discovery',
  uncovered: 'Uncovered',
  covered: 'Covered',
  mixed: 'Passing & failing evidence',
  rejected_only: 'Only rejected evidence'
};
const badge = s => node(
    'span',
    'pill ' +
        (s === 'mixed'             ? 'failed' :
             s === 'rejected_only' ? 'rejected' :
                                     s),
    labels[s] || s);
const statisticsHelp = 'https://github.com/AnarchoSystems/Zucchini/blob/main/Doc/Rules.md#assertion-statistics';
// Declared strictness, strong to weak.
const strictnessLevels = ['exact', 'one_of', 'bounded', 'open_ended', 'unspecified'];
const strictnessLabels = {
  exact: 'Exact', one_of: 'One of', bounded: 'Bounded', open_ended: 'Open-ended', unspecified: 'Unspecified'
};
const strictnessOf = decl => strictnessLevels.includes(decl && decl.strictness) ? decl.strictness : 'unspecified';
function strictnessBadge(decl) {
  const level = strictnessOf(decl);
  const b = node('span', 'strictness ' + level.replace('_', '-'), strictnessLabels[level]);
  b.title = 'Declared strictness of this requirement';
  return b;
}
const businessDeclaration = (rule, event) =>
    rule.kind === 'dsl' ? undefined : rule.requirements.find(o => o.tag === event.tag);
// Observations of business requirements, strong to weak; unspecified, then
// observations without a business requirement, come last.
const strictnessRank = item => {
  const decl = item.rule && item.event ? businessDeclaration(item.rule, item.event) : undefined;
  return decl ? strictnessLevels.indexOf(strictnessOf(decl)) : strictnessLevels.length;
};
const byStrictness = (a, b) => strictnessRank(a) - strictnessRank(b);
const matches = text =>
    String(text).toLowerCase().includes(document.getElementById('search').value.toLowerCase());
const scenarios = () => new Map(state.report.scenarios.map(s => [s.id, s]));
function requirement(rule, decl) {
  const all = rule.events.filter(e => e.kind === 'coverage' && e.tag === decl.tag);
  const byId = scenarios();
  const ids = [...new Set(all.map(e => e.scenario_id))];
  const witnesses = ids.map(id => byId.get(id)).filter(Boolean);
  const accepted = witnesses.filter(s => s.accepted);
  const passing = accepted.filter(s => status(s) === 'passed').length;
  const failing = accepted.filter(s => status(s) === 'failed').length;
  return {
    all, witnesses, accepted, passing, failing,
        label: !accepted.length ? (all.length ? 'rejected_only' : 'uncovered') :
        passing && failing      ? 'mixed' :
        failing                 ? 'failed' :
        passing                 ? 'passed' :
                                  'covered'
  }
}
function source(event) {
  const container = node('div', 'source');
  const scenario = state.report && state.report.scenarios.find(s => s.uri === event.uri);
  const path = event.feature_relative_path || (scenario && scenario.feature_relative_path) ||
      String(event.uri || '').replace(/\\/g, '/').split('/').pop();
  container.append(node('span', '', `${path}:${event.line}${event.column ? ':' + event.column : ''}`));
  if (event.step) container.append(document.createTextNode(' · ' + event.step));
  return container;
}
function witnessDetails(rule, events) {
  const previews = [];
  const details = node('details');
  details.append(
      node('summary', '', `${events.length} observation${events.length === 1 ? '' : 's'}`));
  const byId = scenarios();
  for (const event of events) {
    const w = node('div', 'witness');
    const s = byId.get(event.scenario_id);
    w.append(
        node('strong', '', event.scenario), document.createTextNode(' '),
        badge(s ? status(s) : 'not_run'), node('p', '', event.message), source(event));
    details.append(w);
    if (s) previews.push({w, s, event});
  }
  // Construct source previews on expansion; collapsed evidence stays compact.
  let loaded = false;
  details.addEventListener('toggle', () => {
    if (!details.open || loaded) return;
    loaded = true;
    for (const {w, s, event} of previews) w.append(
        node('p', 'dim', 'Purple marks this observation. Other highlights and hover details belong to this rule only.'),
        inlineScenario(s, rule, event));
  });
  return details
}
function card(title, description, kind) {
  const c = node('article', 'card');
  const head = node('div', 'cardhead');
  const text = node('div');
  text.append(node('h2', '', title), node('p', '', description));
  head.append(text, node('span', 'pill', kind));
  c.append(head);
  return c
}
function table(headers) {
  const t = node('table', 'evidence');
  const head = node('thead');
  const tr = node('tr');
  headers.forEach(h => tr.append(node('th', '', h)));
  head.append(tr);
  t.append(head);
  const body = node('tbody');
  t.append(body);
  return [t, body]
}
function requirementsView(content) {
  for (const rule of state.report.rules) {
    const declarations = rule.requirements.filter(
        o => matches(`${rule.name} ${rule.description} ${o.tag} ${o.description}`));
    if (!declarations.length && !matches(`${rule.name} ${rule.description}`)) continue;
    const c = card(rule.name, rule.description, rule.kind === 'dsl' ? 'DSL rule' : 'Business rule');
    c.className += ' rule-card';
    if (rule.kind === 'dsl') {
      const violated = declarations.map(decl => ({decl, events: rule.events.filter(
          e => e.kind === 'violation' && e.tag === decl.tag)})).filter(o => o.events.length);
      if (!violated.length) continue;
      const [t, body] = table(['Structural constraint', 'Violations']);
      for (const {decl, events} of violated) {
        const row = node('tr');
        const info = node('td');
        info.append(node('strong', '', decl.tag), node('p', '', decl.description));
        const evidence = node('td');
        evidence.append(witnessDetails(rule, events));
        row.append(info, evidence);
        body.append(row);
      }
      c.append(t);
      content.append(c);
      continue;
    }
    const [t, body] = table(['Requirement', 'Coverage / execution', 'Violations']);
    let passing = 0;
    for (const decl of declarations) {
      const data = requirement(rule, decl);
      passing += data.passing > 0 ? 1 : 0;
      const row = node('tr');
      const info = node('td');
      info.append(node('strong', '', decl.tag), strictnessBadge(decl), node('p', '', decl.description));
      const evidence = node('td');
      evidence.append(badge(data.label));
      const counts = data.accepted.reduce((counts, s) => {
        const label = status(s);
        counts[label] = (counts[label] || 0) + 1;
        return counts
      }, {});
      if (data.accepted.length)
        evidence.append(node(
            'p', 'dim',
            Object.entries(counts).map(([s, n]) => `${n} ${labels[s].toLowerCase()}`).join(' · ')));
      if (data.all.length) evidence.append(witnessDetails(rule, data.all));
      const violations = rule.events.filter(e => e.kind === 'violation' && e.tag === decl.tag);
      const v = node('td');
      if (violations.length) {
        v.append(
            node(
                'span',
                'pill ' + (violations.some(e => e.severity === 'error') ? 'failed' : 'uncovered'),
                `${violations.length} observation${violations.length === 1 ? '' : 's'}`),
            witnessDetails(rule, violations))
      } else
        v.append(node('span', 'dim', 'None observed'));
      row.append(info, evidence, v);
      body.append(row)
    }
    const progress = node('div', 'progress');
    const bar = node('i');
    bar.style.width = `${declarations.length ? 100 * passing / declarations.length : 0}%`;
    progress.append(bar);
    c.append(progress, t);
    content.append(c)
  }
}
function scenarioObservations(s, line, onlyRule = null) {
  return (onlyRule ? [onlyRule] : state.report.rules).flatMap(rule => rule.events
      .filter(e => e.scenario_id === s.id && e.uri === s.uri && Number(e.cell?.line ?? e.line) === line)
      .map(event => ({rule, event})));
}
// Cells of a Gherkin table row as [start, end) offsets between unescaped pipes.
function tableCells(text) {
  const pipes = [];
  for (let index = 0; index < text.length; ++index) {
    if (text[index] === '\\') ++index;
    else if (text[index] === '|') pipes.push(index);
  }
  return pipes.slice(1).map((end, index) => ({ start: pipes[index] + 1, end }));
}
// The whole span between a cell's pipes, including padding. Gherkin places an
// empty cell's column on its closing pipe.
function cellRange(text, column) {
  return tableCells(text).find(cell => cell.start <= column - 1 && column - 1 <= cell.end);
}
// Annotated cells of one source line: rule observations, unjustified facts and
// ignored columns.
function lineCells(s, line, text, onlyRule) {
  const cells = new Map();
  const add = (column, item) => {
    const range = cellRange(text, Number(column));
    if (!range) return;
    if (!cells.has(range.start)) cells.set(range.start, { ...range, items: [] });
    cells.get(range.start).items.push(item);
  };
  for (const rule of onlyRule ? [onlyRule] : state.report.rules) for (const event of rule.events) {
    if (event.scenario_id === s.id && event.uri === s.uri && Number(event.cell?.line) === line) add(event.cell.column, { kind: event.kind, rule, event });
  }
  if (!onlyRule) for (const fact of s.unjustified_facts || []) {
    if (fact.cell && Number(fact.line) === line) add(fact.column, { kind: 'unjustified', fact });
  }
  for (const step of s.steps || []) {
    const table = step.argument;
    if (table?.kind !== 'dataTable') continue;
    for (const locations of table.cellLocations || []) for (const key of table.ignoredKeys || []) {
      const location = Array.isArray(locations) ? locations[Number(key)] : locations?.[key];
      if (Number(location?.line) === line) add(location.column, { kind: 'ignored' });
    }
  }
  return [...cells.values()].sort((a, b) => a.start - b.start);
}
function describeCellItem(item) {
  if (item.kind === 'ignored') return 'Ignored: the step definition ignores this column, so it asserts nothing.';
  if (item.kind === 'unjustified') return `Not tied to a requirement: '${item.fact.cell.key}' in row ${item.fact.cell.row + 1}. No business rule claims it as coverage.`;
  const decl = businessDeclaration(item.rule, item.event);
  return `${item.rule.name}${item.event.tag ? ' / ' + item.event.tag : ''}${
      decl ? ' [' + strictnessLabels[strictnessOf(decl)] + ']' : ''} · ${item.event.kind}${
      item.event.severity ? ' (' + item.event.severity + ')' : ''}: ${item.event.message || ''}`;
}
function lineText(text, cells) {
  const span = node('span', 'line-text');
  if (!cells.length) { span.textContent = text; return span; }
  let offset = 0;
  for (const cell of cells) {
    span.append(document.createTextNode(text.slice(offset, cell.start)));
    const status = ['violation', 'unjustified', 'coverage', 'ignored'].find(kind => cell.items.some(item => item.kind === kind));
    const annotated = node('span', 'cell ' + status, text.slice(cell.start, cell.end));
    annotated.title = [...cell.items].sort(byStrictness).map(describeCellItem).join('\n');
    span.append(annotated);
    offset = cell.end;
  }
  span.append(document.createTextNode(text.slice(offset)));
  return span;
}
// untiedOnly highlights only the assertions no requirement claims.
function inlineScenario(s, onlyRule = null, selectedEvent = null, untiedOnly = false) {
  const block = node('div', 'scenario-source');
  const lines = state.report.sources && state.report.sources[s.uri];
  if (!lines) {
    block.append(node('p', 'dim', 'Source snapshot unavailable.'));
    for (const step of s.steps || []) block.append(node('pre', '', step.text));
    return block;
  }
  const next = state.report.scenarios.filter(other => other.uri === s.uri && other.line > s.line)
      .reduce((end, other) => Math.min(end, other.line - 1), lines.length);
  const steps = s.steps || [];
  // Include background steps separately, without intervening scenarios.
  const selected = new Set();
  for (const step of steps.filter(step => step.line < s.line)) {
    selected.add(step.line);
    const indent = lines[step.line - 1]?.search(/\S/) ?? 0;
    for (let i = step.line; i < s.line - 1 && lines[i].trim() && lines[i].search(/\S/) > indent; ++i)
      selected.add(i + 1);
  }
  const scenarioIndent = lines[s.line - 1]?.search(/\S/) ?? 0;
  let docString = false;
  for (let line = s.line; line <= next; ++line) {
    const text = lines[line - 1];
    if (/^\s*("""|```)/.test(text)) docString = !docString;
    // Stop before the following rule or scenario's tags; keyword names may be localized.
    if (line > s.line && !docString && text.trim() && !text.trim().startsWith('#') &&
        text.search(/\S/) <= scenarioIndent && (text.trim().startsWith('@') || text.includes(':'))) break;
    selected.add(line);
  }
  const ordered = [...selected].filter(line => line > 0 && line <= lines.length).sort((a,b) => a-b);
  while (ordered.length && !lines[ordered[ordered.length-1]-1].trim()) ordered.pop();
  let previous = 0;
  for (const line of ordered) {
    if (previous && line > previous + 1) block.append(node('div', 'dim', '…'));
    previous = line;
    const observations = untiedOnly ? [] : scenarioObservations(s, line, onlyRule);
    // Table rows highlight their cells instead of the whole line.
    const cells = lineCells(s, line, lines[line-1], onlyRule).map(cell => untiedOnly
        ? {...cell, items: cell.items.filter(item => item.kind === 'unjustified')} : cell).filter(cell => cell.items.length);
    // Facts the scenario asserts without any business rule claiming them. A
    // table step is unjustified while any of its cells is.
    const stepLine = fact => Number(fact.cell ? s.steps?.[fact.step_index]?.line ?? fact.line : fact.line);
    const unjustified = onlyRule ? [] : (s.unjustified_facts || []).filter(f => stepLine(f) === line);
    const owned = untiedOnly ? [] : (onlyRule ? [onlyRule] : state.report.rules).flatMap(rule => rule.events)
        .filter(e => e.scenario_id === s.id && e.uri === s.uri && e.cell && Number(e.line) === line);
    const violation = [...observations.map(o => o.event), ...owned].some(e => e.kind === 'violation');
    const row = node('div', 'source-line' + (cells.length ? '' : violation ? ' violation' : unjustified.length ? ' unjustified'
        : observations.length || owned.length ? ' coverage' : ''));
    if (observations.some(o => o.event === selectedEvent)) row.className += ' selected-observation';
    row.append(node('span', 'line-number', line), lineText(lines[line-1], cells));
    // Outline instances show substituted text in addition to the original source.
    const step = steps.find(step => step.line === line);
    if (step && lines[line-1].includes('<')) row.append(node('span', 'dim', '→ ' + step.text));
    if (observations.length || unjustified.length) {
      row.tabIndex = 0;
      const count = observations.length + unjustified.length;
      row.append(node('span', 'dim', count + ' observation' + (count === 1 ? '' : 's')));
      const popup = node('div', 'observations');
      for (const fact of unjustified) {
        const observation = node('div', 'observation');
        observation.append(node('strong', '', 'Not tied to a requirement'),
            node('p', '', fact.cell ? `Cell '${fact.cell.key}' in row ${fact.cell.row + 1}` : 'Outcome step'),
            node('p', 'dim', 'No business rule claims this asserted fact as coverage.'));
        popup.append(observation);
      }
      for (const {rule, event} of [...observations].sort(byStrictness)) {
        const observation = node('div', 'observation' + (event === selectedEvent ? ' selected-observation' : ''));
        const declaration = rule.requirements.find(o => o.tag === event.tag);
        const summary = node('p', '', event.kind + (event.severity ? ' · ' + event.severity : '') + (event.tag ? ' · ' + event.tag : ''));
        // The badge sits next to the requirement it describes.
        if (businessDeclaration(rule, event)) summary.append(strictnessBadge(declaration));
        observation.append(node('strong', '', rule.name), summary, node('p', '', event.message));
        if (declaration) observation.append(node('p', 'dim', declaration.description));
        popup.append(observation);
      }
      row.append(popup);
    }
    block.append(row);
  }
  return block;
}
function scenarioEvents(s) {
  return state.report.rules.flatMap(rule => rule.events.filter(e => e.scenario_id === s.id));
}
function scenarioView(content) {
  const matching = state.report.scenarios.filter(s => matches(`${s.name} ${s.feature} ${s.gherkin_rule}`));
  for (const [title, description, participates] of [
      ['Scenarios with rule observations', 'Recorded coverage or violation observations.', true],
      ['Scenarios without rule observations', 'No coverage or violation observations were recorded. Silent structural checks may still have run.', false]]) {
    const group = matching.filter(s => Boolean(scenarioEvents(s).length) === participates);
    if (!group.length) continue;
    const section = node('section');
    section.append(node('h2', '', `${title} (${group.length})`), node('p', '', description));
    content.append(section);
    featureGroups(section, group);
  }
}
// Scenarios as Gherkin writes them: by feature file, then Gherkin rule, in
// source order.
function featureGroups(content, scenarios) {
  const features = new Map();
  for (const s of scenarios) {
    if (!features.has(s.uri)) features.set(s.uri, []);
    features.get(s.uri).push(s);
  }
  for (const [uri, members] of features) {
    members.sort((a, b) => a.line - b.line);
    const feature = node('section', 'feature-group');
    const heading = node('h3', '', members.find(s => s.feature)?.feature || String(uri).replace(/\\/g, '/').split('/').pop());
    feature.append(heading, source({uri, line: Math.min(...members.map(s => s.line)), feature_relative_path: members[0].feature_relative_path}));
    let rule = null, ruleSection = null;
    for (const s of members) {
      if (!s.gherkin_rule) { rule = null; scenarioCards(feature, [s]); continue; }
      if (s.gherkin_rule !== rule) {
        rule = s.gherkin_rule;
        ruleSection = node('section', 'rule-group');
        ruleSection.append(node('h4', '', 'Rule: ' + rule));
        feature.append(ruleSection);
      }
      scenarioCards(ruleSection, [s]);
    }
    content.append(feature);
  }
}
function scenarioCards(content, scenarios) {
  for (const s of scenarios) {
    if (!matches(`${s.name} ${s.feature} ${s.gherkin_rule}`)) continue;
    const r = resultFor(s);
    const c = card(s.name, `${s.feature}${s.gherkin_rule ? ' / ' + s.gherkin_rule : ''}`, labels[status(s)] || status(s));
    c.className += ' scenario-card';
    const info = node('div', 'notice');
    info.append(source(s), badge(status(s)), node('p', 'dim', 'Highlighted lines have rule observations. Hover or focus a line to inspect them. Unmarked lines have no recorded observations.'));
    if (r && r.duration_ms !== undefined) info.append(node('span', 'dim', `${r.duration_ms} ms`));
    c.append(info, inlineScenario(s));
    if (r && r.messages && r.messages.length) {
      const details = node('details');
      details.append(node('summary', '', 'Test output'));
      for (const part of r.messages) {
        const output = node('div', 'witness');
        if (part.gherkin_location) output.append(source(part.gherkin_location));
        if (part.file) output.append(source({uri: part.file, line: part.line, feature_relative_path: part.feature_relative_path}));
        output.append(node('pre', '', part.message));
        details.append(output);
      }
      c.append(details)
    }
    content.append(c);
  }
}
function violationView(content) {
  const byId = scenarios();
  for (const rule of state.report.rules) {
    const events = rule.events.filter(
        e => e.kind === 'violation' && matches(`${rule.name} ${e.tag} ${e.message} ${e.scenario}`));
    if (!events.length) continue;
    const c = card(rule.name, rule.description, `${events.length} violations`);
    c.className += ' scenario-card';
    for (const e of events) {
      const info = node('div', 'notice');
      const declaration = rule.requirements.find(o => o.tag === e.tag);
      info.append(node('strong', '', e.tag), document.createTextNode(' '),
          node('span', 'pill ' + (e.severity === 'error' ? 'failed' : 'uncovered'), e.severity),
          node('p', '', declaration ? declaration.description : ''),
          node('p', '', e.message), node('strong', '', e.scenario), source(e),
          node('p', 'dim', 'Purple marks this violation. Hover highlights to inspect observations from this rule.'));
      c.append(info);
      const scenario = byId.get(e.scenario_id);
      if (scenario) c.append(inlineScenario(scenario, rule, e));
    }
    content.append(c);
  }
}
// An asserted fact names a table cell or, otherwise, the asserting step.
function factLabel(s, fact) {
  if (fact.cell) return `'${fact.cell.key}' in row ${fact.cell.row + 1}`;
  const step = s.steps && s.steps[fact.step_index];
  return step ? step.text : `Step ${fact.step_index + 1}`;
}
function statisticsView(content) {
  const report = state.report;
  const summary = report.assertion_justification;
  const intro = node('section', 'intro');
  const help = node('p');
  const more = node('a', '', 'assertion statistics guide');
  more.href = statisticsHelp;
  more.target = '_blank';
  more.rel = 'noopener';
  help.append(document.createTextNode('For further information, see the '), more, document.createTextNode('.'));
  intro.append(node('h2', '', 'Assertion statistics'),
      node('p', '', 'These statistics are advisory only. They describe how scenario assertions relate to declared requirements; no value, ratio or percentage among them should be used as a hard quality target.'),
      help);
  content.append(intro);
  if (!summary) {
    content.append(node('div', 'empty', 'This report records no asserted facts. Rebuild with a current Zucchini to collect them.'));
    return;
  }
  const silent = report.scenarios.filter(s => s.asserted_facts === 0);
  const metrics = node('div', 'metrics');
  for (const [label, count] of [
           ['Asserted facts', summary.asserted_facts],
           ['Tied to a requirement', summary.asserted_facts - summary.unjustified_facts],
           ['Not tied to a requirement', summary.unjustified_facts],
           ['Scenarios without assertions', summary.scenarios_without_assertions ?? silent.length]]) {
    const metric = node('div', 'metric');
    metric.append(node('strong', '', count), node('span', '', label));
    metrics.append(metric);
  }
  content.append(metrics);

  const declared = report.rules.filter(rule => rule.kind !== 'dsl').flatMap(rule =>
      rule.requirements.map(decl => ({rule, decl})));
  const strictness = card('Requirement strictness',
      'How narrowly each requirement pins down the outcomes it accepts, as its rule declares.',
      `${declared.length} requirements`);
  const levels = node('div', 'strictness-summary');
  for (const level of strictnessLevels) {
    const count = declared.filter(({decl}) => strictnessOf(decl) === level).length;
    if (!count) continue;
    const entry = node('span');
    entry.append(strictnessBadge({strictness: level}), document.createTextNode(String(count)));
    levels.append(entry);
  }
  strictness.children[0].children[0].append(levels);
  const [requirementsTable, requirementRows] = table(['Requirement', 'Strictness', 'Claimed by scenarios']);
  const ranked = declared.filter(({rule, decl}) => matches(`${rule.name} ${decl.tag} ${decl.description}`))
      .sort((a, b) => strictnessLevels.indexOf(strictnessOf(a.decl)) - strictnessLevels.indexOf(strictnessOf(b.decl)));
  for (const {rule, decl} of ranked) {
    const data = requirement(rule, decl);
    const row = node('tr');
    const info = node('td');
    info.append(node('strong', '', decl.tag), node('p', 'dim', rule.name));
    const level = node('td');
    level.append(strictnessBadge(decl));
    const claimed = node('td');
    if (data.all.length)
      claimed.append(document.createTextNode(`${data.witnesses.length} scenario${data.witnesses.length === 1 ? '' : 's'} · ${
          data.all.length} coverage observation${data.all.length === 1 ? '' : 's'}`));
    else
      claimed.append(badge('uncovered'));
    row.append(info, level, claimed);
    requirementRows.append(row);
  }
  if (ranked.length) strictness.append(requirementsTable);
  content.append(strictness);

  const untied = report.scenarios.filter(s => (s.unjustified_facts || []).length &&
      matches(`${s.name} ${s.feature} ${s.gherkin_rule}`))
      .sort((a, b) => b.unjustified_facts.length - a.unjustified_facts.length || String(a.name).localeCompare(String(b.name)));
  const untiedCard = card('Assertions not tied to a requirement',
      'No requirement explains why these assertions were made. This could indicate a tacit requirement that has not been made explicit yet.',
      `${summary.unjustified_facts} fact${summary.unjustified_facts === 1 ? '' : 's'}`);
  if (untied.length) untiedCard.append(node('div', 'notice', 'Amber marks the assertions no requirement claims; other assertions are not highlighted. Hover or focus a line for details.'));
  for (const s of untied) {
    const info = node('div', 'notice');
    const count = s.unjustified_facts.length;
    info.append(node('strong', '', s.name), document.createTextNode(' '),
        node('span', 'pill uncovered', `${count} not tied to a requirement`), source(s));
    untiedCard.append(info, inlineScenario(s, null, null, true));
  }
  content.append(untiedCard);

  const shown = silent.filter(s => matches(`${s.name} ${s.feature} ${s.gherkin_rule}`));
  if (shown.length) {
    const silentCard = card('Scenarios without assertions',
        'These scenarios have no outcome step, so they assert nothing.', `${shown.length} scenarios`);
    const [silentTable, silentRows] = table(['Scenario']);
    for (const s of shown) {
      const row = node('tr');
      const info = node('td');
      info.append(node('strong', '', s.name), source(s));
      row.append(info);
      silentRows.append(row);
    }
    silentCard.append(silentTable);
    content.append(silentCard);
  }
}
function render(force = false) {
  if (!state.report) return;
  const snapshot = JSON.stringify([state.report, [...state.results.values()]]);
  if (!force && snapshot === state.snapshot) return;
  state.snapshot = snapshot;
  const report = state.report;
  const diagnostics = document.getElementById('diagnostics');
  diagnostics.replaceChildren();
  if (report.discovery_diagnostics && report.discovery_diagnostics.length) {
    const c = card(
        report.discovery_success ? 'Discovery diagnostics' : 'Discovery failed',
        'Fix these diagnostics and rebuild to rediscover scenarios', 'Discovery');
    for (const d of report.discovery_diagnostics) c.append(node('pre', '', d.message));
    diagnostics.append(c)
  }
  document.title = `${report.fixture} · Zucchini`;
  document.getElementById('title').textContent = `${report.fixture} · Business requirements`;
  document.getElementById('subtitle').textContent =
      `${report.rules.length} rules · ${report.scenarios.length} scenarios · Discovered ${
          new Date(report.discovered_at_ms).toLocaleString()}`;
  const requirements = report.rules.filter(rule => rule.kind !== 'dsl').flatMap(rule => rule.requirements.map(o => requirement(rule, o)));
  const metrics = document.getElementById('metrics');
  metrics.replaceChildren();
  for (const [label, count] of [
           ['Business requirements', requirements.length],
           ['With passing evidence', requirements.filter(o => o.passing > 0).length],
           ['Without accepted coverage', requirements.filter(o => !o.accepted.length).length],
           ['Scenario failures', report.scenarios.filter(s => status(s) === 'failed').length], [
             'Rule errors',
             report.rules.reduce(
                 (n, r) => n +
                     r.events.filter(e => e.kind === 'violation' && e.severity === 'error').length,
                     0)
           ]]) {
    const metric = node('div', 'metric');
    metric.append(node('strong', '', count), node('span', '', label));
    metrics.append(metric)
  }
  const content = document.getElementById('content');
  content.replaceChildren();
  if (state.view === 'requirements')
    requirementsView(content);
  else if (state.view === 'scenarios')
    scenarioView(content);
  else if (state.view === 'statistics')
    statisticsView(content);
  else
    violationView(content);
  if (!content.children.length) content.append(node('div', 'empty', 'No matching observations.'));
}
function load(path) {
  return new Promise(resolve => {
    const script = document.createElement('script');
    let timer;
    const done = () => {
      clearTimeout(timer);
      script.remove();
      resolve()
    };
    script.onload = done;
    script.onerror = done;
    script.src = path + '?updated=' + Date.now();
    timer = setTimeout(done, 2000);
    document.head.append(script)
  })
}
async function refresh() {
  if (document.hidden) {
    setTimeout(refresh, 2500);
    return
  }
  await load('rules.js');
  await load('ctest-results.js');
  render();
  setTimeout(refresh, 2500)
}
function importCTest(xml) {
  if (!state.report) throw new Error('Wait for discovery data before importing results.');
  const document = new DOMParser().parseFromString(xml, 'application/xml');
  if (document.querySelector('parsererror') || !document.querySelector('testsuite'))
    throw new Error('Select a CTest JUnit XML report.');
  const imported = new Map();
  for (const test of document.querySelectorAll('testcase')) {
    const name = test.getAttribute('name');
    const scenario = state.report.scenarios.find(s => {
      const identity = `${state.report.fixture}.${s.test_name}`;
      // CMake versions format GoogleTest value parameters differently.
      const parameterized = name && name.match(/^(.*?)\s+# GetParam\(\) = (.*)$/);
      return name === identity || name === `${identity}/${s.test_name}` ||
          (parameterized && parameterized[1] === identity && parameterized[2] === s.test_name);
    });
    if (!scenario) continue;
    const failure = test.querySelector('failure, error');
    const skipped = test.querySelector('skipped');
    const output = test.querySelector('system-out');
    imported.set(identity(state.report.fixture, scenario.test_name), {
      fixture: state.report.fixture, test_name: scenario.test_name,
      discovery_id: state.report.discovery_id,
      status: failure ? 'failed' : skipped ? 'skipped' : 'passed',
      duration_ms: Number(test.getAttribute('time') || 0) * 1000,
      messages: failure ? [{message: failure.textContent || (output && output.textContent) || 'Test failed'}] : []
    });
  }
  state.results = imported;
  render();
  return imported.size;
}
window.zucchiniCTest = xml => importCTest(xml);
document.querySelectorAll('[data-view]').forEach(button => button.addEventListener('click', () => {
  state.view = button.dataset.view;
  document.querySelectorAll('[data-view]').forEach(b => b.classList.toggle('active', b === button));
  render(true)
}));
document.getElementById('ctest-results').addEventListener('change', async event => {
  const file = event.target.files[0];
  if (!file) return;
  const message = document.getElementById('import-status');
  try {
    const count = importCTest(await file.text());
    message.textContent = `${count} matching results loaded from ${file.name}. Import results from the same build.`;
  } catch (error) {
    message.textContent = error.message;
  }
});
document.getElementById('search').addEventListener('input', () => render(true));
refresh();
</script></body></html>
)ZUCCHINI_REPORT"
