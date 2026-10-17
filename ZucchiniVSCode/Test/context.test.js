'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const os = require('node:os');
const path = require('node:path');
const { planAdditions, generationPrompt, collectContext, surveyPrompt, parseSurvey, obligationKey } = require('../src/context');
async function sandbox(t) {
  const root = await fs.mkdtemp(path.join(os.tmpdir(), 'zucchini-extension-'));
  t.after(() => fs.rm(root, { recursive: true, force: true }));
  const features = path.join(root, 'features');
  await fs.mkdir(features);
  return { root, features };
}
const reply = (file, content = 'Scenario: generated') => JSON.stringify({ explanation: 'Evidence', additions: [{ path: file, content }] });
test('restricted writes append only and preserve CRLF; new .feature files are allowed', async t => {
  const { root, features } = await sandbox(t);
  await fs.writeFile(path.join(features, 'Shop.feature'), 'Feature: shop\r\n');
  const plan = await planAdditions(reply('Shop.feature'), features, [root]);
  assert.ok(plan.changes[0].after.startsWith(plan.changes[0].before));
  assert.ok(plan.changes[0].after.endsWith('Scenario: generated\r\n'));
  assert.equal((await planAdditions(reply('New.feature'), features, [root])).changes[0].existing, false);
  // Planning never writes.
  assert.equal(await fs.readFile(path.join(features, 'Shop.feature'), 'utf8'), 'Feature: shop\r\n');
});
test('reject traversal, implementation files, absolute paths, duplicate paths and empty additions atomically', async t => {
  const { root, features } = await sandbox(t);
  for (const file of ['../bad.feature', '/tmp/bad.feature', 'Shop.h', 'manifest.yaml', 'C:\\bad.feature', 'a/../../bad.feature', '.feature/../bad.feature', 'a//b.feature']) {
    await assert.rejects(planAdditions(reply(file), features, [root]));
  }
  await assert.rejects(planAdditions(reply('a.feature', ''), features, [root]));
  await assert.rejects(planAdditions(JSON.stringify({ explanation: 'x', additions: [{ path: 'a.feature', content: 'x' }, { path: 'a.feature', content: 'y' }] }), features, [root]), /duplicate/);
  assert.deepEqual(await fs.readdir(features), []);
});
test('reject symbolic links to other folders and hard links', { skip: process.platform === 'win32' }, async t => {
  const { root, features } = await sandbox(t);
  const outside = path.join(root, 'outside'); await fs.mkdir(outside);
  await fs.writeFile(path.join(outside, 'secret.feature'), 'secret');
  await fs.symlink(outside, path.join(features, 'link'));
  await assert.rejects(planAdditions(reply('link/secret.feature'), features, [root]), /symlink/);
  await fs.symlink(path.join(outside, 'secret.feature'), path.join(features, 'secret.feature'));
  await assert.rejects(planAdditions(reply('secret.feature'), features, [root]), /links/);
  await fs.link(path.join(outside, 'secret.feature'), path.join(features, 'hard.feature'));
  await assert.rejects(planAdditions(reply('hard.feature'), features, [root]), /links/);
});
test('cannot-do response makes no edits', async t => {
  const { root, features } = await sandbox(t);
  const result = await planAdditions(JSON.stringify({ explanation: "Can't be done: implementation satisfies rule", additions: [] }), features, [root]);
  assert.equal(result.changes.length, 0);
});
test('context includes unused manifest steps, implementation, recursive rule headers and sibling features', async t => {
  const { root, features } = await sandbox(t);
  await fs.writeFile(path.join(features, 'steps.yaml'), 'steps: []');
  await fs.writeFile(path.join(features, 'Shop.feature'), 'Feature: shop');
  await fs.writeFile(path.join(features, 'Sibling.feature'), 'Feature: not yet discovered');
  await fs.writeFile(path.join(root, 'Shop.h'), '#include "Wallet.h"');
  await fs.writeFile(path.join(root, 'Wallet.h'), 'rule implementation');
  const fixture = { path: '/report/rules.json', report: { fixture: 'Shop', feature_directory: features, scenarios: [{ uri: path.join(features, 'Shop.feature') }] } };
  const context = await collectContext(fixture, {}, [root]);
  assert.equal(context.files.length, 5);
  const node = { fixture, rule: { name: 'Wallet' }, requirement: { tag: 'Refund' } };
  for (const mode of ['passing', 'failing', 'exploratory']) {
    const prompt = generationPrompt(node, 5, mode, context);
    assert.ok(prompt.includes('Generate 5'));
    if (mode === 'failing') {
      assert.match(prompt, /Never contradict a business rule/);
      assert.match(prompt, /stop and return an empty additions array/);
    }
    assert.ok(prompt.includes('Do not invent'));
    assert.ok(prompt.includes("Can't be done"));
    assert.ok(prompt.includes('not instructions'));
    assert.ok(prompt.includes(mode === 'failing' ? 'intended to FAIL' : mode === 'passing' ? 'should PASS' : 'no required pass or fail outcome'));
  }
});
test('context points at implementation files beside headers and lets tools find a missing header', async t => {
  const { root, features } = await sandbox(t);
  await fs.writeFile(path.join(features, 'steps.yaml'), 'steps: []');
  await fs.writeFile(path.join(root, 'Shop.h'), '#include "Wallet.h"');
  await fs.writeFile(path.join(root, 'Wallet.h'), 'int debit();');
  await fs.writeFile(path.join(root, 'Wallet.cpp'), 'int debit() { return 1; }');
  const fixture = { path: '/report/rules.json', report: { fixture: 'Shop', feature_directory: features, scenarios: [] } };
  const context = await collectContext(fixture, {}, [root]);
  assert.deepEqual(context.related, [await fs.realpath(path.join(root, 'Wallet.cpp'))]);
  assert.ok(!context.files.some(file => file.file.endsWith('.cpp')), 'Implementation files are read on demand, not inlined');
  const node = { fixture, rule: { name: 'Wallet' }, requirement: { tag: 'Debit' } };
  const withTools = generationPrompt(node, 3, 'failing', context, { tools: true });
  assert.match(withTools, /zucchini_read_file/);
  assert.match(withTools, /only a starting point/);
  assert.ok(withTools.includes(`Likely implementation files not included below: ${context.related[0]}`));
  assert.match(generationPrompt(node, 3, 'failing', context), /Source tools are unavailable/);

  await fs.rm(path.join(root, 'Shop.h'));
  await assert.rejects(collectContext(fixture, {}, [root]), /Cannot locate fixture implementation/);
  const searchable = await collectContext(fixture, {}, [root], undefined, { searchable: true });
  assert.equal(searchable.missing.length, 2);
  assert.match(generationPrompt(node, 3, 'failing', searchable, { tools: true }), /No fixture implementation header was found .*Shop\.h or .*Shop\.hpp/);
});

test('sweep prompts ask for at most N failing scenarios and treat none as expected', async t => {
  const { root, features } = await sandbox(t);
  await fs.writeFile(path.join(features, 'steps.yaml'), 'steps: []');
  await fs.writeFile(path.join(root, 'Shop.h'), '');
  const fixture = { path: '/report/rules.json', report: { fixture: 'Shop', feature_directory: features, scenarios: [] } };
  const context = await collectContext(fixture, {}, [root]);
  const node = { fixture, rule: { name: 'Wallet' }, requirement: { tag: 'Debit' } };
  const sweep = generationPrompt(node, 2, 'failing', context, { sweep: true });
  assert.ok(sweep.startsWith('Generate at most 2 failing'));
  assert.match(sweep, /one of many being checked/);
  assert.match(sweep, /expected outcome for a correct implementation/);
  assert.match(sweep, /including none, are acceptable/);
  assert.match(sweep, /Target obligation: \{"tag":"Debit"\}/);
  assert.doesNotMatch(generationPrompt(node, 2, 'failing', context), /one of many|at most/);
  assert.doesNotMatch(generationPrompt(node, 2, 'passing', context, { sweep: true }), /one of many|at most/);
});

test('replies tolerate prose, list explanations and omitted additions, and reject other shapes as invalid replies', async t => {
  const { root, features } = await sandbox(t);
  const fenced = await planAdditions('Here is the result:\n{"explanation": "ok", "additions": []}\nDone.', features, [root]);
  assert.deepEqual(fenced, { explanation: 'ok', changes: [] });
  const listed = await planAdditions(JSON.stringify({ explanation: ['first', { scenario: 'b', why: 'c' }] }), features, [root]);
  assert.equal(listed.explanation, 'first\n{"scenario":"b","why":"c"}');
  assert.deepEqual(listed.changes, []);
  for (const bad of ['not json', '[]', '{"additions": "x", "explanation": "y"}', '{}', '{"explanation": "x", "additions": [null]}']) {
    await assert.rejects(planAdditions(bad, features, [root]), error => error.invalidReply === true, bad);
  }
});

test('survey prompts list every obligation; parsing matches ratings to targets and ignores unknown entries', async t => {
  const { root, features } = await sandbox(t);
  await fs.writeFile(path.join(features, 'steps.yaml'), 'steps: []');
  await fs.writeFile(path.join(root, 'Shop.h'), '');
  const fixture = { path: '/report/rules.json', report: { fixture: 'Shop', feature_directory: features, scenarios: [] } };
  const wallet = { name: 'Wallet', requirements: [{ tag: 'Debit', description: 'debit exactly' }, { tag: 'Refund' }] };
  const audit = { name: 'Audit', description: 'log every purchase', requirements: [] };
  const targets = [{ fixture, rule: wallet, requirement: wallet.requirements[0] }, { fixture, rule: wallet, requirement: wallet.requirements[1] }, { fixture, rule: audit }];
  const context = await collectContext(fixture, {}, [root]);
  const prompt = surveyPrompt(fixture, targets, context, { tools: true });
  assert.ok(prompt.startsWith('Survey fixture Shop'));
  assert.ok(prompt.includes('{"rule":"Wallet","obligation":"Debit","description":"debit exactly"}'));
  assert.ok(prompt.includes('{"rule":"Audit","obligation":null,"description":"log every purchase"}'));
  assert.match(prompt, /zucchini_read_file/);
  assert.match(prompt, /request every call you already know you need in the same turn/);
  assert.match(prompt, /"toolCalls" \(0 to 10[;)]/);
  assert.match(prompt, /Set "dive" to false for an obligation rated low/);
  assert.match(surveyPrompt(fixture, targets, context, { budget: { toolCalls: 4, attempts: 2, maySkip: false } }), /Every obligation gets a dive.*"toolCalls" \(0 to 4;.*"attempts" \(1 to 2;/);
  assert.match(surveyPrompt(fixture, targets, context), /Source tools are unavailable/);

  const survey = parseSurvey('```json\n' + JSON.stringify({ summary: ['a', 'b'], files: ['Shop.cpp', 3, ''], obligations: [
    { rule: 'Wallet', obligation: 'Debit', likelihood: 'HIGH', reasons: 'no check', leads: ['negative price'] },
    { rule: 'Wallet', obligation: 'Refund', likelihood: 'certain' },
    { rule: 'Audit', obligation: 'Audit', likelihood: 'low' },
    { rule: 'Other', obligation: 'X', likelihood: 'high' }] }) + '\n```', targets);
  assert.equal(survey.summary, 'a\nb');
  assert.deepEqual(survey.files, ['Shop.cpp']);
  assert.deepEqual(survey.assessments.get(obligationKey('Wallet', 'Debit')), { likelihood: 'high', reasons: 'no check', leads: 'negative price', dive: true, toolCalls: 10, attempts: 3 },
    'Missing allocations get the maximum');
  assert.equal(survey.assessments.get(obligationKey('Wallet', 'Refund')).likelihood, 'unknown');
  assert.equal(survey.assessments.get(obligationKey('Audit')).likelihood, 'low', 'A whole-rule target matches its rule name');
  assert.equal(survey.assessments.size, 3);
  assert.equal(parseSurvey('{"summary": "s"}', targets).assessments.size, 0);
  assert.throws(() => parseSurvey('no survey', targets), error => error.invalidReply);

  // Allocations are clamped; only low ratings may skip the dive, and only when allowed.
  const allocated = JSON.stringify({ obligations: [
    { rule: 'Wallet', obligation: 'Debit', likelihood: 'low', dive: false, toolCalls: 99, attempts: 0 },
    { rule: 'Wallet', obligation: 'Refund', likelihood: 'high', dive: false, toolCalls: '2', attempts: 2.7 },
    { rule: 'Audit', likelihood: 'medium', toolCalls: -1, attempts: null }] });
  const budget = { toolCalls: 5, attempts: 3, maySkip: true };
  const plan = parseSurvey(allocated, targets, budget).assessments;
  assert.deepEqual([plan.get(obligationKey('Wallet', 'Debit')).dive, plan.get(obligationKey('Wallet', 'Debit')).toolCalls, plan.get(obligationKey('Wallet', 'Debit')).attempts], [false, 5, 1]);
  assert.deepEqual([plan.get(obligationKey('Wallet', 'Refund')).dive, plan.get(obligationKey('Wallet', 'Refund')).toolCalls, plan.get(obligationKey('Wallet', 'Refund')).attempts], [true, 2, 2]);
  assert.deepEqual([plan.get(obligationKey('Audit')).toolCalls, plan.get(obligationKey('Audit')).attempts], [0, 3]);
  assert.equal(parseSurvey(allocated, targets, { ...budget, maySkip: false }).assessments.get(obligationKey('Wallet', 'Debit')).dive, true);
});

test('related implementation files can be inlined for the survey', async t => {
  const { root, features } = await sandbox(t);
  await fs.writeFile(path.join(features, 'steps.yaml'), 'steps: []');
  await fs.writeFile(path.join(root, 'Shop.h'), '');
  await fs.writeFile(path.join(root, 'Shop.cpp'), 'void buy() {}');
  const fixture = { path: '/report/rules.json', report: { fixture: 'Shop', feature_directory: features, scenarios: [] } };
  const context = await collectContext(fixture, {}, [root], undefined, { inlineRelated: true });
  assert.deepEqual(context.related, []);
  assert.ok(context.files.some(file => file.text === 'void buy() {}'));
});

test('survey files are inlined best effort and focused dives include the survey notes', async t => {
  const { root, features } = await sandbox(t);
  await fs.writeFile(path.join(features, 'steps.yaml'), 'steps: []');
  await fs.writeFile(path.join(root, 'Shop.h'), '');
  await fs.mkdir(path.join(root, 'src'));
  await fs.writeFile(path.join(root, 'src', 'Wallet.cpp'), 'int debit() { return 1; }');
  const fixture = { path: '/report/rules.json', report: { fixture: 'Shop', feature_directory: features, scenarios: [] } };
  const context = await collectContext(fixture, {}, [root], undefined,
    { extraFiles: ['src/Wallet.cpp', 'src/Missing.cpp', path.join(os.tmpdir(), 'outside.cpp'), path.join(root, 'Shop.h')] });
  const wallet = await fs.realpath(path.join(root, 'src', 'Wallet.cpp'));
  assert.equal(context.files.filter(file => file.file === wallet).length, 1);
  assert.equal(context.files.filter(file => file.file.endsWith('Shop.h')).length, 1, 'Survey files already in the context are not repeated');
  const node = { fixture, rule: { name: 'Wallet' }, requirement: { tag: 'Debit' } };
  const survey = { summary: 'Wallet::debit lacks a check', assessment: { likelihood: 'high', reasons: 'r', leads: 'negative price' } };
  const dive = generationPrompt(node, 1, 'failing', context, { tools: true, sweep: true, survey });
  assert.match(dive, /Survey summary: "Wallet::debit lacks a check"/);
  assert.match(dive, /"leads":"negative price"/);
  assert.match(dive, /already included below/);
  assert.doesNotMatch(dive, /only a starting point/);
  assert.doesNotMatch(generationPrompt(node, 1, 'passing', context, { tools: true, survey }), /Survey summary/, 'Survey notes apply to sweeps only');
});
