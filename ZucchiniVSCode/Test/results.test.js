'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const path = require('node:path');
const os = require('node:os');
const { loadResults, scenarioStatus, aggregateStatus } = require('../src/results');
test('persisted results follow discovery identity, including after a rebuild or restart', async t => {
  const root = await fs.mkdtemp(path.join(os.tmpdir(), 'zucchini-results-'));
  t.after(() => fs.rm(root, { recursive: true, force: true }));
  const scenario = { id: 0, test_name: 'buy', result_key: '1234567890abcdef', accepted: true, sourceState: 'current' };
  const fixture = { path: path.join(root, 'rules.json'), report: { fixture: 'Shop', discovery_id: 'old-build', scenarios: [scenario] } };
  await fs.mkdir(path.join(root, 'results'));
  const file = path.join(root, 'results', scenario.result_key + '.json');
  await fs.writeFile(file, JSON.stringify({ fixture: 'Shop', test_name: 'buy', discovery_id: 'old-build', status: 'passed' }));
  fixture.results = await loadResults(fixture);
  assert.equal(scenarioStatus(fixture, scenario), 'passed');
  fixture.report.discovery_id = 'new-build';
  assert.equal(scenarioStatus(fixture, scenario), 'stale');
  await fs.writeFile(file, JSON.stringify({ fixture: 'Shop', test_name: 'buy', discovery_id: 'new-build', status: 'failed' }));
  fixture.results = await loadResults(fixture);
  assert.equal(scenarioStatus(fixture, scenario), 'failed');
  assert.equal(scenarioStatus(fixture, { ...scenario, sourceState: 'modified' }), 'stale');
});
test('discovery failures never masquerade as red execution failures', () => {
  const fixture = { report: { discovery_success: false, discovery_id: 'new' } };
  assert.equal(scenarioStatus(fixture, { id: 0, accepted: false }), 'rejected');
  assert.equal(scenarioStatus(fixture, { id: 1, accepted: true }), 'not_run');
  assert.equal(aggregateStatus(['rejected', 'not_run']), 'not_run');
  assert.equal(aggregateStatus(['passed', 'not_run']), 'passed');
  assert.equal(aggregateStatus(['passed', 'failed']), 'failed');
  assert.equal(aggregateStatus(['stale', 'rejected']), 'stale');
});

test('fresh local outcomes take precedence over legacy disk results', () => {
  const scenario = { id: 0, accepted: true, sourceState: 'current' };
  const fixture = { report: { discovery_id: 'current' }, results: new Map([
    [0, { discovery_id: 'older', status: 'passed' }]
  ]) };
  for (const status of ['running', 'failed', 'passed', 'skipped', 'cancelled', 'incomplete']) {
    assert.equal(scenarioStatus(fixture, scenario, { discovery_id: 'current', status }), status);
  }
});

test('CTest snapshots reload selected outcomes after restart, supersede legacy files, and become stale after rediscovery', async t => {
  const { runTests } = require('../src/runner');
  const root = await fs.mkdtemp(path.join(os.tmpdir(), 'zucchini CTest status '));
  t.after(() => fs.rm(root, { recursive: true, force: true }));
  const scenarios = ['buy', 'refund', 'skip', 'absent'].map((name, id) => ({
    id, test_name: name, accepted: true, sourceState: 'current', result_key: '1234567890abcdef'
  }));
  const report = { fixture: 'Shop', discovery_id: 'current', scenarios };
  const fixture = { path: path.join(root, 'rules.json'), report };
  await fs.mkdir(path.join(root, 'results'));
  await fs.writeFile(path.join(root, 'results', '1234567890abcdef.json'), JSON.stringify({
    fixture: 'Shop', test_name: 'buy', discovery_id: 'old', status: 'passed'
  }));
  const tests = scenarios.slice(0, 3).map(s => ({ name: `custom prefix ${s.test_name}`, command: ['Shop.exe', `--gtest_filter=Shop.${s.test_name}`] }));
  await runTests({ ctest: 'ctest', base: [], tests, reportDirectory: root, report,
    token: { isCancellationRequested: false },
    async execute(executable, args) {
      await fs.writeFile(args.at(-1), `<testsuite>
        <testcase name="custom prefix buy" status="fail"><failure/></testcase>
        <testcase name="custom prefix refund" status="run"/>
        <testcase name="custom prefix skip" status="notrun"><skipped/></testcase>
      </testsuite>`);
      return { code: 8, output: '' };
    }
  });
  fixture.results = await loadResults(fixture);
  assert.deepEqual(scenarios.map(s => scenarioStatus(fixture, s)), ['failed', 'passed', 'skipped', 'not_run']);
  report.discovery_id = 'rebuilt';
  fixture.results = await loadResults(fixture);
  assert.equal(scenarioStatus(fixture, scenarios[0]), 'stale');
  // A malformed new snapshot must not resurrect legacy success.
  await fs.writeFile(path.join(root, 'ctest-results.json'), 'broken JSON');
  fixture.results = await loadResults(fixture);
  assert.equal(scenarioStatus(fixture, scenarios[0]), 'not_run');
});
