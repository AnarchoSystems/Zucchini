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
test('flat tree includes every scenario and retains rule/tag membership', () => {
  assert.equal(model.children({ kind: 'fixture', fixture }, 'scenarios').length, 3);
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
  for (const menu of Object.values(manifest.contributes.menus)) for (const item of menu) assert.ok(commands.has(item.command));
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


test('fixture filters include and exclude names with literal text and wildcards', () => {
  const fixtures = ['Shop', 'LegacyShop', 'Inventory', 'Shop.v2', 'Wallet[old]', 'Shop+'].map(fixture => ({ report: { fixture } }));
  const names = expression => model.filterFixtures(fixtures, expression).map(f => f.report.fixture);
  assert.equal(names('').length, fixtures.length);
  assert.deepEqual(names('shop, inventory, !legacy'), ['Shop', 'Inventory', 'Shop.v2', 'Shop+']);
  assert.deepEqual(names('!Shop'), ['Inventory', 'Wallet[old]']);
  assert.deepEqual(names('Sh?p, !Legacy'), ['Shop', 'Shop.v2', 'Shop+']);
  assert.deepEqual(names('Wallet[old]'), ['Wallet[old]']);
  assert.deepEqual(names('Shop+'), ['Shop+']);
  assert.deepEqual(names('Shop.v?'), ['Shop.v2']);
  assert.deepEqual(names('Inv*ry'), ['Inventory']);
  assert.deepEqual(names('Missing'), []);
  assert.equal(names(' , ! , ').length, fixtures.length);
});
