'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const model = require('../src/model');
const fixture = { path: '/build/rules.json', report: {
  schema_version: '2.0.0', fixture: 'Shop', feature_directory: '/features',
  scenarios: [0, 1, 2].map(id => ({ id, name: `Scenario ${id}`, test_name: `S${id}`, uri: '/features/Shop.feature', line: id + 1 })),
  rules: [{ name: 'Wallet', requirements: [{ tag: 'Debit' }, { tag: 'Refund' }], events: [
    { kind: 'coverage', scenario_id: 0, tag: 'Debit', uri: '/features/Shop.feature', line: 4 },
    { kind: 'coverage', scenario_id: 0, tag: 'Debit', uri: '/features/Shop.feature', line: 5 },
    { kind: 'violation', scenario_id: 1, tag: 'Refund' }
  ] }, { name: 'Protocol', requirements: [{ tag: 'Complete' }], events: [] }]
} };
test('rules retain unwitnessed declarations, deduplicate scenarios and separate unassigned scenarios', () => {
  const rules = model.children({ kind: 'fixture', fixture }, 'rules');
  assert.deepEqual(rules.map(r => r.kind), ['rule', 'rule', 'unassigned']);
  const obligations = model.children(rules[0], 'rules');
  assert.equal(obligations.length, 2);
  assert.equal(model.children(obligations[0], 'rules').length, 1);
  assert.equal(model.children(obligations[1], 'rules')[0].scenario.id, 1);
  assert.equal(model.witnesses(fixture.report, rules[0].rule, obligations[1].requirement).length, 0);
  assert.equal(model.children(rules[2], 'rules')[0].scenario.id, 2);
  assert.equal(model.children(rules[1], 'rules').length, 1);
});
test('scenario tree groups every scenario by feature and retains rule/tag membership', () => {
  const [feature] = model.children({ kind: 'fixture', fixture }, 'scenarios');
  assert.equal(feature.kind, 'feature');
  assert.equal(model.children(feature, 'scenarios').length, 3);
  assert.deepEqual(model.participation(fixture.report, fixture.report.scenarios[0])[0].tags, ['Debit']);
  assert.equal(model.stepObservations(fixture.report, '/features/Shop.feature', 4)[0].rule.name, 'Wallet');
});
test('bulk execution is unique, includes violating scenarios, and excludes unrelated ones', () => {
  assert.deepEqual(model.scenariosUnder({ kind: 'rule', fixture, rule: fixture.report.rules[0] }).map(s => s.id), [0, 1]);
});
test('unsupported schema and duplicate scenario IDs fail clearly', () => {
  assert.throws(() => model.parseReport('{}'), /schema/);
  const duplicate = structuredClone(fixture.report);
  duplicate.scenarios.push(duplicate.scenarios[0]);
  assert.throws(() => model.parseReport(JSON.stringify(duplicate)), /duplicate/);
});
// Use a checked-in report-shaped fixture, rather than requiring a preexisting build.
test('manifest contributions reference registered commands and use scenario-only navigation', () => {
  const manifest = JSON.parse(fs.readFileSync(path.join(__dirname, '../package.json'), 'utf8'));
  const commands = new Set(manifest.contributes.commands.map(c => c.command));
  const submenus = new Set(manifest.contributes.submenus.map(s => s.id));
  for (const menu of Object.values(manifest.contributes.menus)) for (const item of menu) {
    assert.ok(item.submenu ? submenus.has(item.submenu) : commands.has(item.command));
  }
  // The view menu offers each grouping; the active one's entry carries a check mark.
  const views = manifest.contributes.menus['zucchini.view'];
  const groupings = manifest.contributes.configuration.properties['zucchini.grouping'].enum;
  assert.deepEqual(views.map(item => item.command), ['zucchini.showRules', 'zucchini.showRules.checked', 'zucchini.showScenarios', 'zucchini.showScenarios.checked',
    'zucchini.showStatistics', 'zucchini.showStatistics.checked']);
  for (const [index, grouping] of groupings.entries()) {
    const [plain, checked] = views.slice(2 * index, 2 * index + 2);
    assert.equal(plain.when, `config.zucchini.grouping != ${grouping}`);
    assert.equal(checked.when, `config.zucchini.grouping == ${grouping}`);
    const title = command => manifest.contributes.commands.find(c => c.command === command).title;
    assert.equal(title(checked.command), `✓ ${title(plain.command)}`);
    assert.ok(manifest.contributes.menus.commandPalette.some(item => item.command === checked.command && item.when === 'false'));
  }
});

test('version range accepts supported reports and rejects missing, future and malformed versions', () => {
  for (const version of [undefined, null, 2, '2', '2.0', '02.0.0', '2.0.0.0', 'v2.0.0', ' 2.0.0', '2.0.0 ', '2.0.0-alpha.01', '1.9.9', '3.0.0', '2.1.0-beta.1']) {
    const report = { ...fixture.report, schema_version: version };
    assert.throws(() => model.parseReport(JSON.stringify(report)), /expected >=2\.0\.0 <3\.0\.0/);
  }
  assert.throws(() => model.parseReport('null'), /Unsupported/);
  assert.equal(model.parseReport(JSON.stringify(fixture.report)).schema_version, '2.0.0');
  for (const version of ['2.0.1', '2.1.0', '2.10.0', '2.99.99', '2.0.0+build.7']) {
    assert.ok(model.supportsReportVersion(version), version);
  }
  // Adding fields within a supported version must not break the reader.
  assert.equal(model.parseReport(JSON.stringify({ ...fixture.report, extra: true })).extra, true);
});

test('extension range covers the current versioned schema selected by the runtime', () => {
  const runtimeCmake = fs.readFileSync(path.join(__dirname, '../../ZucchiniRuntime/CMakeLists.txt'), 'utf8');
  const schemaName = runtimeCmake.match(/schemas\/(rules-report\.v\d+\.\d+\.\d+\.schema\.json)/)?.[1];
  assert.ok(schemaName, 'Runtime must select a versioned rule report schema');
  const schema = JSON.parse(fs.readFileSync(path.join(__dirname, '../../schemas', schemaName), 'utf8'));
  const current = schema.properties.schema_version.const;
  assert.equal(schemaName, `rules-report.v${current}.schema.json`);
  assert.ok(model.supportsReportVersion(current), `Extension range must cover runtime schema version ${current}`);
  assert.equal(schema.$id.split('/').at(-1), schemaName);
});

test('DSL rules are absent from trees, scenario membership and step annotations', () => {
  const copy = structuredClone(fixture);
  copy.report.rules.push({ name: 'DSL protocol', kind: 'dsl', requirements: [{ tag: 'Order' }], events: [{ kind: 'violation', tag: 'Order', scenario_id: 2, uri: '/features/Shop.feature', line: 99 }] });
  const nodes = model.children({ kind: 'fixture', fixture: copy }, 'rules');
  assert.ok(!nodes.some(n => n.rule?.kind === 'dsl'));
  assert.equal(model.participation(copy.report, copy.report.scenarios[2]).length, 0);
  assert.equal(model.stepObservations(copy.report, '/features/Shop.feature', 99).length, 0);
});


test('duplicate raw fixture names show their executable in the fixture picker', () => {
  const { fixtureLabel } = require('../src/model');
  const one = { report: { fixture: 'RawFixture', executable: 'One.exe' } };
  const two = { report: { fixture: 'RawFixture', executable: 'Two.exe' } };
  assert.equal(fixtureLabel(one, [one]), 'RawFixture');
  assert.equal(fixtureLabel(one, [one, two]), 'One.exe/RawFixture');
});

test('obligations enumerate every business-rule requirement, or the rule itself when it has none', () => {
  const { obligations } = require('../src/model');
  const wallet = { name: 'Wallet', requirements: [{ tag: 'Debit' }, { tag: 'Refund' }], events: [] };
  const audit = { name: 'Audit', requirements: [], events: [] };
  const dsl = { name: 'Steps', kind: 'dsl', requirements: [{ tag: 'Syntax' }], events: [] };
  const fixture = { report: { rules: [wallet, dsl, audit] } };
  assert.deepEqual(obligations(fixture).map(node => [node.kind, node.rule.name, node.requirement?.tag]),
    [['requirement', 'Wallet', 'Debit'], ['requirement', 'Wallet', 'Refund'], ['rule', 'Audit', undefined]]);
  assert.ok(obligations(fixture).every(node => node.fixture === fixture));
  assert.deepEqual(obligations(fixture, wallet).map(node => node.requirement.tag), ['Debit', 'Refund']);
});

test('scenarios expand to lines with rule observations or unjustified assertions', () => {
  const report = { ...fixture.report, scenarios: [
    { ...fixture.report.scenarios[0], steps: [{ text: 'stock', line: 4 }, { text: 'the basket contains', line: 5 }],
      unjustified_facts: [{ step_index: 1, line: 6, column: 9, cell: { row: 0, key: 'note' } }, { step_index: 1, line: 5, column: 5 }] },
    ...fixture.report.scenarios.slice(1)] };
  report.rules = [{ ...report.rules[0], events: [...report.rules[0].events,
    { kind: 'coverage', scenario_id: 0, tag: 'Debit', uri: '/features/Shop.feature', line: 5, cell: { row: 0, key: 'item', line: 6, column: 5 } }] },
    report.rules[1]];
  const scenarios = { ...fixture, report };
  const [feature] = model.children({ kind: 'fixture', fixture: scenarios }, 'scenarios');
  const [scenario] = model.children(feature, 'scenarios');
  const lines = model.children(scenario, 'scenarios');
  // The cell's observation and fact belong to row 6 under the step on line 5.
  assert.deepEqual(lines.map(l => [l.line, l.observations.length, l.unjustified.length, l.rows.length]), [[4, 1, 0, 0], [5, 1, 1, 1]]);
  const [row] = model.children(lines[1], 'scenarios');
  assert.deepEqual([row.line, row.observations.length, row.unjustified.length], [6, 1, 1]);
  assert.equal(model.factLabel(scenario.scenario, row.unjustified[0]), "'note' in row 1");
  assert.equal(model.factLabel(scenario.scenario, lines[1].unjustified[0]), 'the basket contains');
  assert.deepEqual(model.scenariosUnder(lines[0]).map(s => s.id), [0]);
  assert.equal(model.lineStatus(lines[0]), 'covered');
  assert.equal(model.lineStatus(lines[1]), 'unjustified');
  assert.equal(model.lineStatus({ ...lines[1], unjustified: [], rows: [{ ...row, unjustified: [] }] }), 'covered');
  // Cell observations sit on their row; rule hierarchies keep scenarios as leaves.
  assert.equal(model.stepObservations(report, '/features/Shop.feature', 6).length, 1);
  assert.deepEqual(model.children(scenario, 'rules'), []);
});

test('table cells are located by index, including empty and escaped cells', () => {
  const row = '      | tea | a \\| b |   | x |';
  assert.deepEqual(model.tableCells(row).map(c => row.slice(c.start, c.end)), [' tea ', ' a \\| b ', '   ', ' x ']);
  assert.equal(model.cellIndex(row, 9), 0);
  assert.equal(model.cellIndex(row, 15), 1);
  // Gherkin reports an empty cell at its closing pipe.
  assert.equal(model.cellIndex(row, 26), 2);
  assert.equal(model.cellIndex(row, 1), -1);
  const tea = model.cellRange(row, 0);
  assert.equal(row.slice(tea.start, tea.end), ' tea ');
  const empty = model.cellRange(row, 2);
  assert.equal(row.slice(empty.start, empty.end), '   ');
  assert.equal(model.cellRange(row, 9), undefined);
  // Re-indented rows keep their cell indices.
  const shifted = '  ' + row;
  const moved = model.cellRange(shifted, model.cellIndex(row, 9));
  assert.equal(shifted.slice(moved.start, moved.end), ' tea ');
});

test('ignored cells come from table steps of keyed and headerless tables', () => {
  const scenario = { steps: [
    { text: 'no table' },
    { text: 'keyed', argument: { kind: 'dataTable', rows: [{ a: '1', shelf: 'A1' }, { a: '2', shelf: 'B1' }], ignoredKeys: ['shelf'],
      cellLocations: [{ a: { line: 4, column: 9 }, shelf: { line: 4, column: 13 } }, { a: { line: 5, column: 9 }, shelf: { line: 5, column: 13 } }] } },
    { text: 'positional', argument: { kind: 'dataTable', rows: [['x', 'y']], ignoredKeys: ['1'],
      cellLocations: [[{ line: 7, column: 9 }, { line: 7, column: 13 }]] } },
    { text: 'unlocated', argument: { kind: 'dataTable', rows: [{ shelf: 'C1' }], ignoredKeys: ['shelf'] } }] };
  assert.deepEqual(model.ignoredCells(scenario).map(c => [c.step_index, c.row, c.key, c.line, c.column]),
    [[1, 0, 'shelf', 4, 13], [1, 1, 'shelf', 5, 13], [2, 0, '1', 7, 13]]);
});

test('observations order strong to weak and statistics rank requirements by strictness', () => {
  const loose = { name: 'Loose', requirements: [{ tag: 'Note' }], events: [] };
  const dsl = { name: 'Shape', kind: 'dsl', requirements: [{ tag: 'Outcome', strictness: 'exact' }], events: [] };
  const tight = { name: 'Tight', requirements: [{ tag: 'Total', strictness: 'bounded' }, { tag: 'Line', strictness: 'exact' }],
    events: [{ kind: 'coverage', tag: 'Line', scenario_id: 0 }, { kind: 'violation', tag: 'Total', scenario_id: 1 }] };
  const items = [{ rule: dsl, event: { tag: 'Outcome' } }, { rule: loose, event: { tag: 'Note' } }, { kind: 'ignored' },
    { rule: tight, event: { tag: 'Total' } }, { rule: tight, event: { tag: 'Line' } }];
  assert.deepEqual([...items].sort(model.byStrictness).map(item => item.event?.tag ?? item.kind),
    ['Line', 'Total', 'Note', 'Outcome', 'ignored'], 'DSL constraints have no strictness');
  assert.equal(model.strictnessOf({ strictness: 'stricter' }), 'unspecified');
  const report = { rules: [loose, dsl, tight], assertion_justification: { asserted_facts: 3, unjustified_facts: 2 },
    scenarios: [{ id: 0, name: 'b', asserted_facts: 2, unjustified_facts: [{ step_index: 0 }] },
      { id: 1, name: 'a', asserted_facts: 0, unjustified_facts: [] },
      { id: 2, name: 'c', asserted_facts: 1, unjustified_facts: [{ step_index: 0 }, { step_index: 1 }] }] };
  const stats = model.assertionStatistics({ report });
  assert.deepEqual(stats.requirements.map(r => [r.requirement.tag, r.strictness, r.witnesses, r.observations]),
    [['Line', 'exact', 1, 1], ['Total', 'bounded', 0, 0], ['Note', 'unspecified', 0, 0]]);
  assert.deepEqual(stats.levels, [{ level: 'exact', count: 1 }, { level: 'bounded', count: 1 }, { level: 'unspecified', count: 1 }]);
  assert.deepEqual(stats.untied.map(s => s.name), ['c', 'b'], 'Most facts not tied to a requirement first');
  assert.deepEqual(stats.silent.map(s => s.name), ['a']);
});

test('scenarios group by feature file, then Gherkin rule, in source order', () => {
  const scenario = (id, uri, line, rule, extra = {}) => ({ id, name: `S${id}`, uri, line, feature: uri === '/a.feature' ? 'Alpha' : '', gherkin_rule: rule, ...extra });
  const grouped = { report: { rules: [], scenarios: [
    scenario(0, '/a.feature', 9, 'Refunds'), scenario(1, '/a.feature', 3, ''), scenario(2, '/b.feature', 2, ''),
    scenario(3, '/a.feature', 5, 'Refunds'), scenario(4, '/a.feature', 12, 'Purchases')] } };
  // A pending scenario keeps the feature name its file declares.
  grouped.scenarios = [...grouped.report.scenarios, scenario('pending', '/a.feature', 7, 'Refunds', { sourceState: 'pending', displayLine: 7 })];
  const features = model.children({ kind: 'fixture', fixture: grouped }, 'scenarios');
  assert.deepEqual(features.map(f => [f.uri, f.name]), [['/a.feature', 'Alpha'], ['/b.feature', '']]);
  const items = model.children(features[0], 'scenarios');
  assert.deepEqual(items.map(item => item.kind === 'scenario' ? item.scenario.id : item.name), [1, 'Refunds', 'Purchases']);
  assert.deepEqual(model.children(items[1], 'scenarios').map(item => item.scenario.id), [3, 'pending', 0]);
  assert.deepEqual(model.scenariosUnder(features[0]).map(s => s.id), [1, 3, 'pending', 0, 4]);
  assert.deepEqual(model.scenariosUnder(items[2]).map(s => s.id), [4]);
});
