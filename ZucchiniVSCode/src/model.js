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

// Comma-separated name filters, like Test Explorer: include terms are ORed,
// exclusions win, and plain terms match substrings. No regular expressions.
function filterFixtures(fixtures, expression) {
  const terms = expression.split(',').map(term => term.trim()).filter(Boolean);
  const pattern = term => new RegExp(term.split('*').map(part =>
    part.split('?').map(text => text.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')).join('.')
  ).join('.*'), 'i');
  const include = terms.filter(term => !term.startsWith('!')).map(pattern);
  const exclude = terms.filter(term => term.startsWith('!') && term.length > 1).map(term => pattern(term.slice(1)));
  return fixtures.filter(fixture => (!include.length || include.some(test => test.test(fixture.report.fixture))) &&
    !exclude.some(test => test.test(fixture.report.fixture)));
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

function children(node, mode) {
  const { fixture, rule, requirement } = node;
  const report = fixture.report;
  const scenarios = allScenarios(fixture);
  const scenarioNode = scenario => ({ kind: 'scenario', fixture, scenario });
  if (node.kind === 'fixture') {
    if (mode === 'scenarios') return scenarios.map(scenarioNode);
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
  return [];
}

function scenariosUnder(node) {
  if (node.kind === 'scenario') return [node.scenario];
  if (node.kind === 'fixture') return allScenarios(node.fixture);
  if (node.kind === 'unassigned') return allScenarios(node.fixture).filter(s => !participation(node.fixture.report, s).length);
  const ids = new Set(eventsFor(node.fixture.report, node.rule, node.requirement).map(e => e.scenario_id));
  return allScenarios(node.fixture).filter(s => ids.has(s.id));
}

function stepObservations(report, uri, line) {
  return businessRules(report).flatMap(rule => rule.events.filter(e => e.uri === uri && e.line === line).map(event => ({ rule, event })));
}

module.exports = { filterFixtures, businessRules, allScenarios, supportsReportVersion, parseReport, eventsFor, witnesses, participation, children, scenariosUnder, stepObservations };
