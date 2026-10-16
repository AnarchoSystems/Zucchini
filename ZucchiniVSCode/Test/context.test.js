'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const os = require('node:os');
const path = require('node:path');
const { planAdditions, generationPrompt, collectContext } = require('../src/context');
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
