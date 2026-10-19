'use strict';

const model = require('./model');

const statisticsHelp = 'https://github.com/AnarchoSystems/Zucchini/blob/main/Doc/Rules.md#assertion-statistics';
const untiedNote = 'No requirement explains why these assertions were made. This could indicate a tacit requirement that has not been made explicit yet.';

const escape = text => String(text ?? '').replace(/[&<>"']/g, character =>
  ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' })[character]);
const plural = (count, noun) => `${count} ${noun}${count === 1 ? '' : 's'}`;
const level = strictness => strictness.replace('_', '-');
const badge = strictness => `<span class="badge ${level(strictness)}">${escape(model.strictnessLabels[strictness])}</span>`;

// Links back to the editor are buttons, so the webview needs no navigation.
const openScenario = (scenario, text) =>
  `<button class="link" data-open="${scenario.id}">${escape(text)}</button>`;

function requirementsSection(stats) {
  if (!stats.requirements.length) return '<p class="muted">This fixture declares no business requirements.</p>';
  const levels = stats.levels.map(({ level: strictness, count }) => `<span class="level">${badge(strictness)} ${count}</span>`).join('');
  const rows = stats.requirements.map(entry => `<tr>
      <td><strong>${escape(entry.requirement.tag)}</strong><div class="muted">${escape(entry.rule.name)}</div></td>
      <td>${badge(entry.strictness)}</td>
      <td class="numeric">${entry.witnesses ? plural(entry.witnesses, 'scenario') : '<span class="muted">Uncovered</span>'}</td>
    </tr>`).join('');
  return `<div class="levels">${levels}</div>
    <table><thead><tr><th>Requirement</th><th>Strictness</th><th class="numeric">Claimed by</th></tr></thead><tbody>${rows}</tbody></table>`;
}

function untiedSection(stats) {
  if (!stats.untied.length) return '<p class="muted">Every assertion is tied to a requirement.</p>';
  return `<p>${escape(untiedNote)}</p>` + stats.untied.map(scenario => {
    const facts = model.unjustifiedFacts(scenario).map(fact =>
      `<li><button class="link" data-open="${scenario.id}" data-line="${fact.line}">${escape(model.factLabel(scenario, fact))}</button></li>`).join('');
    return `<div class="scenario">
        <div class="scenario-head">${openScenario(scenario, scenario.name)}<span class="count">${model.unjustifiedFacts(scenario).length}</span></div>
        <ul>${facts}</ul>
      </div>`;
  }).join('');
}

// The page shown by the Statistics view: the selected fixture's assertion
// statistics, styled with the editor theme. colors maps each strictness to its
// badge background and text color.
function renderStatistics({ fixture, fixtures, colors, nonce, cspSource }) {
  const csp = `default-src 'none'; style-src ${cspSource} 'nonce-${nonce}'; script-src 'nonce-${nonce}';`;
  const badgeStyles = Object.entries(colors).map(([strictness, { background, text }]) =>
    `.badge.${level(strictness)}{background:${background};color:${text}}`).join('');
  let body;
  if (!fixture) {
    body = '<p class="muted">No fixture reports yet. Build your project, then refresh.</p>';
  } else {
    const stats = model.assertionStatistics(fixture);
    const summary = stats.summary;
    const header = `<header>
        <div><div class="eyebrow">Selected fixture</div><h1>${escape(model.fixtureLabel(fixture, fixtures))}</h1></div>
        <button class="secondary" data-action="select">Change fixture</button>
      </header>
      <p class="note">These statistics are advisory only. They describe how scenario assertions relate to declared requirements;
        no value, ratio or percentage among them should be used as a hard quality target. For further information, see the
        <button class="link" data-action="help">assertion statistics guide</button>.</p>`;
    body = header + (!summary ? '<p class="muted">This report records no asserted facts. Rebuild with a current Zucchini to collect them.</p>' : `
      <section class="metrics">
        <div class="metric"><strong>${summary.asserted_facts}</strong><span>Asserted facts</span></div>
        <div class="metric"><strong>${summary.asserted_facts - summary.unjustified_facts}</strong><span>Tied to a requirement</span></div>
        <div class="metric"><strong>${summary.unjustified_facts}</strong><span>Not tied to a requirement</span></div>
        <div class="metric"><strong>${summary.scenarios_without_assertions ?? stats.silent.length}</strong><span>Scenarios without assertions</span></div>
      </section>
      <section><h2>Assertions not tied to a requirement</h2>${untiedSection(stats)}</section>
      <section><h2>Requirement strictness</h2>
        <p class="muted">How narrowly each requirement pins down the outcomes it accepts, as its rule declares.</p>
        ${requirementsSection(stats)}</section>
      <section><h2>Scenarios without assertions</h2>${stats.silent.length
        ? `<p class="muted">These scenarios have no outcome step, so they assert nothing.</p><ul class="plain">${
          stats.silent.map(scenario => `<li>${openScenario(scenario, scenario.name)}</li>`).join('')}</ul>`
        : '<p class="muted">Every scenario asserts something.</p>'}</section>`);
  }
  return `<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta http-equiv="Content-Security-Policy" content="${csp}">
<meta name="viewport" content="width=device-width,initial-scale=1">
<style nonce="${nonce}">
body{color:var(--vscode-foreground);font-family:var(--vscode-font-family);font-size:var(--vscode-font-size);padding:4px 14px 24px;line-height:1.5}
header{display:flex;justify-content:space-between;align-items:flex-end;gap:12px;flex-wrap:wrap;padding:6px 0 10px;border-bottom:1px solid var(--vscode-panel-border)}
.eyebrow{font-size:11px;text-transform:uppercase;letter-spacing:.08em;color:var(--vscode-descriptionForeground)}
h1{font-size:20px;font-weight:600;margin:2px 0 0;overflow-wrap:anywhere}
h2{font-size:13px;font-weight:600;margin:22px 0 6px;text-transform:uppercase;letter-spacing:.05em;color:var(--vscode-descriptionForeground)}
.note{color:var(--vscode-descriptionForeground);margin:10px 0 4px}
.muted{color:var(--vscode-descriptionForeground)}
.metrics{display:grid;grid-template-columns:repeat(auto-fit,minmax(120px,1fr));gap:8px;margin-top:14px}
.metric{background:var(--vscode-editorWidget-background);border:1px solid var(--vscode-widget-border,var(--vscode-panel-border));border-radius:6px;padding:10px 12px}
.metric strong{display:block;font-size:22px;font-weight:600;line-height:1.2}
.metric span{font-size:12px;color:var(--vscode-descriptionForeground)}
.scenario{border-left:2px solid var(--vscode-editorWarning-foreground);padding:2px 0 2px 10px;margin:10px 0}
.scenario-head{display:flex;justify-content:space-between;gap:8px;align-items:baseline}
.count{font-size:11px;border-radius:9px;padding:0 7px;background:var(--vscode-badge-background);color:var(--vscode-badge-foreground)}
ul{margin:4px 0 0;padding-left:18px}ul.plain{padding-left:0;list-style:none}li{margin:2px 0}
table{width:100%;border-collapse:collapse;margin-top:6px}th,td{text-align:left;padding:6px 4px;border-top:1px solid var(--vscode-panel-border);vertical-align:top}
th{font-size:11px;font-weight:600;color:var(--vscode-descriptionForeground)}.numeric{text-align:right;white-space:nowrap}
.levels{display:flex;flex-wrap:wrap;gap:4px 12px}.level{white-space:nowrap}
.badge{display:inline-block;border-radius:4px;padding:0 6px;font-size:11px;font-weight:600;line-height:1.7;white-space:nowrap}
${badgeStyles}
button{font:inherit;cursor:pointer}
button.link{background:none;border:none;padding:0;color:var(--vscode-textLink-foreground);text-align:left}
button.link:hover{color:var(--vscode-textLink-activeForeground);text-decoration:underline}
button.secondary{background:var(--vscode-button-secondaryBackground);color:var(--vscode-button-secondaryForeground);border:none;border-radius:3px;padding:4px 10px}
button.secondary:hover{background:var(--vscode-button-secondaryHoverBackground)}
button:focus-visible{outline:1px solid var(--vscode-focusBorder);outline-offset:2px}
</style></head>
<body>${body}
<script nonce="${nonce}">
const vscode = acquireVsCodeApi();
document.addEventListener('click', event => {
  const target = event.target.closest('button');
  if (!target) return;
  if (target.dataset.open !== undefined) {
    vscode.postMessage({ type: 'open', scenario: Number(target.dataset.open), line: target.dataset.line ? Number(target.dataset.line) : undefined });
  } else if (target.dataset.action) vscode.postMessage({ type: target.dataset.action });
});
</script></body></html>`;
}

module.exports = { renderStatistics, statisticsHelp };
