'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const os = require('node:os');
const path = require('node:path');
const { collectContext } = require('../src/context');
const { huntPrompt, huntRevision, parseHunt, planHunt, verifyHunt } = require('../src/hunt');

async function sandbox(t) {
  const root = await fs.mkdtemp(path.join(os.tmpdir(), 'zucchini hunt-'));
  t.after(() => fs.rm(root, { recursive: true, force: true }));
  const features = path.join(root, 'features');
  await fs.mkdir(features);
  await fs.writeFile(path.join(features, 'steps.yaml'), 'steps: []');
  await fs.writeFile(path.join(root, 'Shop.h'), '');
  return { root, features: await fs.realpath(features) };
}

test('the prompt asks for every defect in one reply, with claims, notes and a uniform limit', async t => {
  const { root, features } = await sandbox(t);
  const fixture = { path: '/report/rules.json', report: { fixture: 'Shop', feature_directory: features, scenarios: [] } };
  const wallet = { name: 'Wallet', requirements: [{ tag: 'Debit', description: 'debit exactly' }] };
  const audit = { name: 'Audit', description: 'log every purchase', requirements: [] };
  const targets = [{ fixture, rule: wallet, requirement: wallet.requirements[0] }, { fixture, rule: audit }];
  const context = await collectContext(fixture, {}, [root]);
  const prompt = huntPrompt(fixture, targets, context, { tools: true, limit: 4, iterations: 2 });
  assert.ok(prompt.startsWith('Find implementation defects in fixture Shop'));
  assert.ok(prompt.includes('[{"rule":"Wallet","obligation":"Debit","description":"debit exactly"},{"rule":"Audit","obligation":null,"description":"log every purchase"}]'));
  assert.match(prompt, /at most 4 scenarios/);
  assert.match(prompt, /at most 2 iteration\(s\)/);
  assert.match(prompt, /"notes".*"scenarios".*"obligations"/);
  assert.match(prompt, /empty scenarios array is the expected answer/);
  assert.match(prompt, /zucchini_read_file/);
  assert.match(huntPrompt(fixture, targets, context), /Source tools are unavailable/);

  const reply = parseHunt(JSON.stringify({ notes: 'n', scenarios: [{ path: 'Shop.feature', content: 'Scenario: x', obligations: [{ rule: 'Wallet', obligation: 'Debit' }] }] }), 4);
  const revision = huntRevision('BASE', { iteration: 2, iterations: 3, limit: 4, reply, diagnostics: 'x'.repeat(40000) + 'tail',
    verdicts: [{ entry: reply.entries[0], confirmed: false, reasons: ['test passed'], scenarios: ['x'] }] });
  assert.ok(revision.startsWith('BASE\nThis is iteration 2 of 3.'));
  assert.match(revision, /"notes":"n"/);
  assert.match(revision, /1\. Shop\.feature \(Wallet \/ Debit\): not confirmed: test passed/);
  assert.match(revision, /retract/);
  assert.ok(revision.includes('tail') && !revision.includes('x'.repeat(30001)), 'Diagnostics keep their tail');
});

test('replies tolerate prose and omitted arrays, normalize claims, and cap the scenario count', () => {
  const entry = (name, obligations) => ({ path: 'Shop.feature', content: `Scenario: ${name}`, obligations });
  const parsed = parseHunt('Result:\n' + JSON.stringify({ notes: ['a', 'b'], scenarios: [
    entry('one', [{ rule: 'Wallet', obligation: 'Debit' }, { rule: 'Audit', obligation: '' }, 'junk', { obligation: 'x' }]),
    entry('two', { rule: 'Wallet', obligation: 'Refund' }), entry('three', [])] }), 2);
  assert.equal(parsed.notes, 'a\nb');
  assert.equal(parsed.entries.length, 2);
  assert.equal(parsed.dropped, 1);
  assert.deepEqual(parsed.entries[0].obligations, [{ rule: 'Wallet', obligation: 'Debit' }, { rule: 'Audit', obligation: null }]);
  assert.deepEqual(parsed.entries[1].obligations, [{ rule: 'Wallet', obligation: 'Refund' }]);
  assert.deepEqual(parseHunt('{"notes": "nothing found"}', 5), { notes: 'nothing found', entries: [], dropped: 0 });
  for (const bad of ['no json', '{}', '{"notes": "x", "scenarios": "none"}', '{"scenarios": [{"path": "a.feature"}]}', '{"scenarios": [null]}']) {
    assert.throws(() => parseHunt(bad, 5), error => error.invalidReply === true, bad);
  }
});

test('planning appends in order, tracks each entry\'s lines, and restores retracted files', async t => {
  const { root, features } = await sandbox(t);
  const shop = path.join(features, 'Shop.feature');
  await fs.writeFile(shop, 'Feature: shop\n  Scenario: old\n');
  const entries = [
    { path: 'Shop.feature', content: '  Scenario: a\n    Then x' },
    { path: 'New.feature', content: 'Feature: new\n  Scenario: b' },
    { path: 'Shop.feature', content: '  Scenario: c' }];
  const plan = await planHunt(entries, features, [root], new Map());
  assert.deepEqual(plan.changes.map(change => [path.basename(change.file), change.existing]), [['Shop.feature', true], ['New.feature', false]]);
  assert.equal(plan.changes[0].after, 'Feature: shop\n  Scenario: old\n\n\n  Scenario: a\n    Then x\n\n  Scenario: c\n');
  assert.deepEqual(plan.placements.map(({ first, last }) => [first, last]), [[5, 6], [1, 2], [8, 8]]);
  await assert.rejects(planHunt([{ path: '../x.feature', content: 'Scenario: x' }], features, [root], new Map()), error => error.invalidReply);

  // A later iteration plans relative to the original text and undoes retracted files.
  const created = path.join(features, 'New.feature');
  await fs.writeFile(created, plan.changes[1].after);
  await fs.writeFile(shop, plan.changes[0].after);
  const previous = new Map([[shop, { ...plan.changes[0], created: false }], [created, { ...plan.changes[1], created: true }]]);
  const read = async file => previous.get(file)?.before ?? fs.readFile(file, 'utf8');
  const retracted = await planHunt([], features, [root], previous, read);
  assert.deepEqual(retracted.changes.map(change => [path.basename(change.file), change.after, Boolean(change.remove)]),
    [['Shop.feature', 'Feature: shop\n  Scenario: old\n', false], ['New.feature', '', true]]);
  const revised = await planHunt([{ path: 'New.feature', content: 'Feature: new\n  Scenario: d' }], features, [root], previous, read);
  assert.equal(revised.changes[0].existing, false, 'A file created by an earlier iteration is rewritten from scratch');
  assert.equal(revised.changes[0].after, 'Feature: new\n  Scenario: d\n');
});

test('verification confirms only accepted, covering, violation-free scenarios that fail', () => {
  const file = path.resolve('features', 'Shop.feature');
  const scenario = (id, name, line) => ({ id, name, uri: file, line });
  const report = { rules: [
    { name: 'Wallet', requirements: [{ tag: 'Debit' }, { tag: 'Refund' }], events: [
      { kind: 'coverage', tag: 'Debit', scenario_id: 1 }, { kind: 'coverage', tag: 'Debit', scenario_id: 2 },
      { kind: 'coverage', tag: 'Debit', scenario_id: 3 }, { kind: 'violation', tag: 'Refund', scenario_id: 3, message: 'refund unchecked' }] },
    { name: 'Audit', requirements: [], events: [{ kind: 'coverage', scenario_id: 1 }] },
    { name: 'Sequence', kind: 'dsl', requirements: [], events: [{ kind: 'violation', scenario_id: 4 }] }] };
  const validation = { report, results: [
    { scenario: scenario(1, 'confirmed', 10), outcome: 'failed' }, { scenario: scenario(2, 'passes', 20), outcome: 'passed' },
    { scenario: scenario(3, 'contradicts', 30), outcome: 'failed' }, { scenario: scenario(4, 'protocol', 40), outcome: 'rejected' }] };
  const claim = (rule, obligation = null) => ({ rule, obligation });
  const entries = [
    { path: 'Shop.feature', obligations: [claim('Wallet', 'Debit'), claim('Audit'), claim('Audit', 'Audit')] },
    { path: 'Shop.feature', obligations: [claim('Wallet', 'Debit')] },
    { path: 'Shop.feature', obligations: [claim('Wallet', 'Debit')] },
    { path: 'Shop.feature', obligations: [claim('Wallet', 'Refund'), claim('Wallet', 'Missing'), claim('Nope')] },
    { path: 'Shop.feature', obligations: [] },
    { path: 'Shop.feature', obligations: [claim('Wallet')] }];
  const placements = [[9, 11], [20, 21], [30, 30], [40, 41], [50, 52], [10, 10]].map(([first, last]) => ({ file, first, last }));
  const verdicts = verifyHunt(entries, placements, validation);
  assert.deepEqual(verdicts.map(verdict => verdict.confirmed), [true, false, false, false, false, true]);
  assert.deepEqual(verdicts[0].scenarios, ['confirmed']);
  assert.deepEqual(verdicts[1].reasons, ['test passed']);
  assert.deepEqual(verdicts[2].reasons, ['Wallet / Refund reported a violation: refund unchecked']);
  assert.deepEqual(verdicts[3].reasons, ['rejected during discovery', 'Sequence reported a violation',
    'discovery recorded no coverage of Wallet / Refund', 'unknown obligation Wallet / Missing', 'unknown business rule Nope']);
  assert.deepEqual(verdicts[4].reasons, ['no scenario was discovered from this entry', 'it claims no obligation']);
  assert.deepEqual(verifyHunt([entries[0]], [placements[0]], { ready: false, results: [] })[0].reasons, ['the fixture did not build or discovery failed']);
});
