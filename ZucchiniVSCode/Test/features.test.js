'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { projectReport } = require('../src/features');
const uri = '/features/Shop.feature';
const source = 'Feature: shop\n  Background:\n    Given stock\n  Scenario: buy\n    Then debit\n';
const report = { scenarios: [{ id: 0, name: 'buy', uri, line: 4 }], sources: { [uri]: source.trimEnd().split('\n') } };
test('adding Foo keeps unchanged scenario observations and shows Foo awaiting discovery', async () => {
  const current = source + '\n  Scenario: Foo\n    Then debit\n';
  const result = await projectReport(report, new Map([[uri, current]]));
  assert.equal(result.scenarios[0].sourceState, 'current');
  assert.equal(result.locations.get(uri).get(5), 5);
  assert.equal(result.scenarios[1].name, 'Foo');
  assert.equal(result.scenarios[1].sourceState, 'pending');
});
test('inserted scenarios and comments remap locations without guessing coverage for new steps', async () => {
  const current = source.replace('  Scenario: buy', '  Scenario: Foo\n    Then debit\n\n  # inserted comment\n  Scenario: buy');
  const result = await projectReport(report, new Map([[uri, current.replaceAll('\n', '\r\n')]]));
  assert.equal(result.scenarios[0].displayLine, 8);
  assert.equal(result.locations.get(uri).get(5), 9);
  assert.equal(result.scenarios[1].sourceState, 'pending');
});
test('editing a scenario invalidates its annotations; editing a background invalidates dependents', async () => {
  for (const current of [source.replace('Then debit', 'Then refund'), source.replace('Given stock', 'Given other stock')]) {
    const result = await projectReport(report, new Map([[uri, current]]));
    assert.equal(result.scenarios[0].sourceState, 'modified');
    assert.equal(result.locations.get(uri).has(5), false);
    assert.equal(result.scenarios.length, 1);
  }
});
test('removing a scenario and parse errors are distinct from test failures', async () => {
  assert.equal((await projectReport(report, new Map([[uri, 'Feature: empty\n']]))).scenarios[0].sourceState, 'removed');
  const invalid = await projectReport(report, new Map([[uri, 'not a feature']]));
  assert.equal(invalid.scenarios[0].sourceState, 'unknown');
  assert.equal(invalid.sourceErrors.length, 1);
});
test('localized scenario outlines are parsed and background locations follow their scope', async () => {
  const text = '# language: de\nFunktionalität: Shop\n  Szenariogrundriss: Kaufen\n    Angenommen Preis <preis>\n    Beispiele:\n      | preis |\n      | 7     |\n';
  const data = { scenarios: [{ id: 0, uri, line: 3 }], sources: { [uri]: text.trimEnd().split('\n') } };
  const result = await projectReport(data, new Map([[uri, text]]));
  assert.equal(result.scenarios[0].sourceState, 'current');
});
