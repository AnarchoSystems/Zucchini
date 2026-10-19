'use strict';

const semver = require('semver');
const { zucchiniRuleReport: supportedReportVersions } = require('../package.json');

function supportsReportVersion(version) {
  return typeof version === 'string' && /^\d+\.\d+\.\d+(?:-|\+|$)/.test(version) &&
    !/\s/.test(version) && semver.valid(version) !== null &&
    semver.satisfies(version, supportedReportVersions.versionRange);
}

function parseReport(text) {
  const report = JSON.parse(text);
  if (!report || !supportsReportVersion(report.schema_version)) {
    throw new Error(`Unsupported Zucchini rule report schema version ${JSON.stringify(report?.schema_version) ?? 'missing'}; expected ${supportedReportVersions.versionRange}. Rebuild with a compatible Zucchini version or update the extension.`);
  }
  if (typeof report.fixture !== 'string' ||
      (report.executable !== undefined && typeof report.executable !== 'string') ||
      typeof report.feature_directory !== 'string' || !Array.isArray(report.rules) || !Array.isArray(report.scenarios)) {
    throw new Error('Invalid Zucchini rule report structure');
  }
  const ids = new Set();
  for (const scenario of report.scenarios) {
    if (!Number.isInteger(scenario.id) || ids.has(scenario.id) || typeof scenario.name !== 'string' ||
        typeof scenario.uri !== 'string' || !Number.isInteger(scenario.line) || scenario.line < 1 ||
        typeof scenario.test_name !== 'string') throw new Error('Invalid or duplicate scenario');
    ids.add(scenario.id);
  }
  for (const rule of report.rules) {
    if (typeof rule.name !== 'string' || !Array.isArray(rule.events) || !Array.isArray(rule.requirements) ||
        rule.requirements.some(r => typeof r.tag !== 'string')) throw new Error('Invalid rule declaration');
  }
  return report;
}

function fixtureLabel(fixture, fixtures) {
  const name = fixture.report.fixture;
  return fixture.report.executable && fixtures.some(other => other !== fixture && other.report.fixture === name)
    ? `${fixture.report.executable}/${name}` : name;
}


const businessRules = report => report.rules.filter(rule => rule.kind !== 'dsl');
const allScenarios = fixture => fixture.scenarios || fixture.report.scenarios;

function eventsFor(report, rule, requirement) {
  return (rule ? [rule] : businessRules(report)).flatMap(r => r.events)
    .filter(e => !requirement || e.tag === requirement.tag);
}

function witnesses(report, rule, requirement) {
  const ids = new Set(eventsFor(report, rule, requirement).filter(e => e.kind === 'coverage').map(e => e.scenario_id));
  return report.scenarios.filter(s => ids.has(s.id));
}

function participation(report, scenario) {
  return businessRules(report).flatMap(rule => {
    const events = rule.events.filter(e => e.scenario_id === scenario.id);
    return events.length ? [{ rule, tags: [...new Set(events.map(e => e.tag).filter(Boolean))] }] : [];
  });
}

// Declared requirement strictness, strong to weak.
const strictnessLevels = ['exact', 'one_of', 'bounded', 'open_ended', 'unspecified'];
const strictnessLabels = { exact: 'Exact', one_of: 'One of', bounded: 'Bounded', open_ended: 'Open-ended', unspecified: 'Unspecified' };
const strictnessOf = requirement => strictnessLevels.includes(requirement?.strictness) ? requirement.strictness : 'unspecified';
// The business requirement an observation is evidence for; DSL constraints have none.
const requirementFor = (rule, event) => rule.kind === 'dsl' ? undefined : rule.requirements.find(r => r.tag === event.tag);
// Observations of business requirements, strong to weak; unspecified, then
// observations without a business requirement, come last.
function byStrictness(a, b) {
  const rank = ({ rule, event }) => {
    const requirement = rule && event && requirementFor(rule, event);
    return requirement ? strictnessLevels.indexOf(strictnessOf(requirement)) : strictnessLevels.length;
  };
  return rank(a) - rank(b);
}

// Observations of table cells sit on the cell's row.
const eventLine = event => event.cell?.line ?? event.line;
const unjustifiedFacts = scenario => scenario.unjustified_facts || [];

// An unjustified fact names a table cell or, otherwise, the asserting step.
function factLabel(scenario, fact) {
  if (fact.cell) return `'${fact.cell.key}' in row ${fact.cell.row + 1}`;
  return scenario.steps?.[fact.step_index]?.text ?? `Step ${fact.step_index + 1}`;
}

// The line of the step a fact belongs to; table cells belong to their step.
const stepLine = (scenario, fact) => scenario.steps?.[fact.step_index]?.line ?? fact.line;

// Steps of a scenario with business rule observations or unjustified
// assertions, in source order. Observations and facts of table cells belong to
// their row, listed under the step.
function highlightedLines(report, scenario) {
  const steps = new Map();
  const step = line => {
    if (!steps.has(line)) steps.set(line, { line, observations: [], unjustified: [], rows: new Map() });
    return steps.get(line);
  };
  const row = (owner, line) => {
    const rows = step(owner).rows;
    if (!rows.has(line)) rows.set(line, { line, observations: [], unjustified: [] });
    return rows.get(line);
  };
  for (const rule of businessRules(report)) for (const event of rule.events) {
    if (event.scenario_id !== scenario.id || event.uri !== scenario.uri || !Number.isInteger(event.line)) continue;
    (Number.isInteger(event.cell?.line) ? row(event.line, event.cell.line) : step(event.line)).observations.push({ rule, event });
  }
  for (const fact of unjustifiedFacts(scenario)) {
    (fact.cell ? row(stepLine(scenario, fact), fact.line) : step(fact.line)).unjustified.push(fact);
  }
  const byLine = (a, b) => a.line - b.line;
  return [...steps.values()].map(entry => ({ ...entry, rows: [...entry.rows.values()].sort(byLine) })).sort(byLine);
}

// Cells of columns the step definitions' structs ignore; they assert nothing.
function ignoredCells(scenario) {
  return (scenario.steps || []).flatMap((step, stepIndex) => {
    const table = step.argument;
    if (table?.kind !== 'dataTable' || !table.ignoredKeys?.length) return [];
    return (table.cellLocations || []).flatMap((locations, row) => table.ignoredKeys.map(key => {
      const location = Array.isArray(locations) ? locations[Number(key)] : locations?.[key];
      return location ? { step_index: stepIndex, row, key, line: location.line, column: location.column } : undefined;
    }).filter(Boolean));
  });
}

// A step is unjustified while any of its cells is; violations take precedence.
function lineStatus(entry) {
  const all = [entry, ...(entry.rows || [])];
  if (all.some(e => e.observations.some(({ event }) => event.kind === 'violation'))) return 'violation';
  if (all.some(e => e.unjustified.length)) return 'unjustified';
  return 'covered';
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

// The index of the cell at a 1-based Gherkin column. Gherkin places an empty
// cell's column on its closing pipe.
function cellIndex(text, column) {
  const offset = column - 1;
  return tableCells(text).findIndex(cell => cell.start <= offset && offset <= cell.end);
}

// The cell's whole span between its pipes, including padding.
const cellRange = (text, index) => tableCells(text)[index];

// Scenarios grouped as Gherkin writes them: by feature file, then Gherkin rule.
// Groups and ungrouped scenarios keep their source order.
const scenarioOrder = (a, b) => (a.displayLine ?? a.line) - (b.displayLine ?? b.line);
function featureGroups(fixture) {
  const features = new Map();
  for (const scenario of allScenarios(fixture)) {
    if (!features.has(scenario.uri)) features.set(scenario.uri, { kind: 'feature', fixture, uri: scenario.uri, name: '' });
    features.get(scenario.uri).name ||= scenario.feature || '';
  }
  return [...features.values()];
}
function featureChildren(node) {
  const scenarios = allScenarios(node.fixture).filter(s => s.uri === node.uri).sort(scenarioOrder);
  const result = [], rules = new Map();
  for (const scenario of scenarios) {
    if (!scenario.gherkin_rule) { result.push({ kind: 'scenario', fixture: node.fixture, scenario }); continue; }
    if (!rules.has(scenario.gherkin_rule)) {
      rules.set(scenario.gherkin_rule, { kind: 'gherkinRule', fixture: node.fixture, uri: node.uri, name: scenario.gherkin_rule });
      result.push(rules.get(scenario.gherkin_rule));
    }
  }
  return result;
}
const gherkinRuleScenarios = node => allScenarios(node.fixture)
  .filter(s => s.uri === node.uri && s.gherkin_rule === node.name).sort(scenarioOrder);

function children(node, mode) {
  const { fixture, rule, requirement } = node;
  const report = fixture.report;
  const scenarios = allScenarios(fixture);
  const scenarioNode = scenario => ({ kind: 'scenario', fixture, scenario });
  if (node.kind === 'fixture') {
    if (mode === 'scenarios') return featureGroups(fixture);
    const rules = businessRules(report).map(rule => ({ kind: 'rule', fixture, rule }));
    if (scenarios.some(s => participation(report, s).length === 0)) rules.push({ kind: 'unassigned', fixture });
    return rules;
  }
  if (node.kind === 'rule') {
    const result = rule.requirements.map(requirement => ({ kind: 'requirement', fixture, rule, requirement }));
    // Retain observations with no declared requirement, including untagged violations.
    const tags = new Set(rule.requirements.map(r => r.tag));
    const ids = new Set(rule.events.filter(e => !tags.has(e.tag)).map(e => e.scenario_id));
    result.push(...scenarios.filter(s => ids.has(s.id)).map(scenarioNode));
    return result;
  }
  if (node.kind === 'requirement') {
    const ids = new Set(eventsFor(report, rule, requirement).map(e => e.scenario_id));
    return scenarios.filter(s => ids.has(s.id)).map(scenarioNode);
  }
  if (node.kind === 'unassigned') return scenarios.filter(s => !participation(report, s).length).map(scenarioNode);
  if (node.kind === 'feature') return featureChildren(node);
  if (node.kind === 'gherkinRule') return gherkinRuleScenarios(node).map(scenarioNode);
  if (node.kind === 'scenario' && mode === 'scenarios') {
    return highlightedLines(report, node.scenario).map(entry => ({ kind: 'line', fixture, scenario: node.scenario, ...entry }));
  }
  if (node.kind === 'line') return (node.rows || []).map(entry => ({ kind: 'line', fixture, scenario: node.scenario, ...entry }));
  return [];
}

function scenariosUnder(node) {
  if (node.kind === 'scenario' || node.kind === 'line') return [node.scenario];
  if (node.kind === 'fixture') return allScenarios(node.fixture);
  if (node.kind === 'unassigned') return allScenarios(node.fixture).filter(s => !participation(node.fixture.report, s).length);
  if (node.kind === 'feature') return allScenarios(node.fixture).filter(s => s.uri === node.uri).sort(scenarioOrder);
  if (node.kind === 'gherkinRule') return gherkinRuleScenarios(node);
  const ids = new Set(eventsFor(node.fixture.report, node.rule, node.requirement).map(e => e.scenario_id));
  return allScenarios(node.fixture).filter(s => ids.has(s.id));
}

function stepObservations(report, uri, line) {
  return businessRules(report).flatMap(rule => rule.events.filter(e => e.uri === uri && eventLine(e) === line).map(event => ({ rule, event })));
}

// Advisory assertion statistics of a fixture; summary is absent in reports
// that record no asserted facts.
function assertionStatistics(fixture) {
  const report = fixture.report;
  const scenarios = allScenarios(fixture).filter(s => s.sourceState !== 'pending');
  const requirements = businessRules(report).flatMap(rule => rule.requirements.map(requirement => ({
    rule, requirement, strictness: strictnessOf(requirement),
    witnesses: witnesses(report, rule, requirement).length,
    observations: eventsFor(report, rule, requirement).filter(e => e.kind === 'coverage').length
  }))).sort((a, b) => strictnessLevels.indexOf(a.strictness) - strictnessLevels.indexOf(b.strictness));
  return {
    summary: report.assertion_justification,
    untied: scenarios.filter(s => unjustifiedFacts(s).length)
      .sort((a, b) => unjustifiedFacts(b).length - unjustifiedFacts(a).length || a.name.localeCompare(b.name)),
    silent: scenarios.filter(s => s.asserted_facts === 0),
    requirements,
    levels: strictnessLevels.map(level => ({ level, count: requirements.filter(r => r.strictness === level).length }))
      .filter(entry => entry.count)
  };
}

// Every business-rule obligation of a fixture (or of one rule), in report order.
// Rules that declare no obligations are targeted as a whole.
function obligations(fixture, rule) {
  return (rule ? [rule] : businessRules(fixture.report)).flatMap(r => r.requirements.length
    ? r.requirements.map(requirement => ({ kind: 'requirement', fixture, rule: r, requirement }))
    : [{ kind: 'rule', fixture, rule: r }]);
}

module.exports = { strictnessLevels, strictnessLabels, strictnessOf, requirementFor, byStrictness, assertionStatistics, ignoredCells, cellIndex, cellRange, lineStatus, stepLine, tableCells, eventLine, highlightedLines, unjustifiedFacts, factLabel, obligations, fixtureLabel, businessRules, allScenarios, supportsReportVersion, parseReport, eventsFor, witnesses, participation, children, scenariosUnder, stepObservations };
