'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const os = require('node:os');
const path = require('node:path');
const { createSourceTools, companionFiles, definitions } = require('../src/sources');

async function workspace(t) {
  const base = await fs.realpath(await fs.mkdtemp(path.join(os.tmpdir(), 'zucchini-sources-')));
  t.after(() => fs.rm(base, { recursive: true, force: true }));
  const root = path.join(base, 'project');
  await fs.mkdir(path.join(root, 'src', 'deep'), { recursive: true });
  await fs.mkdir(path.join(root, 'node_modules', 'pkg'), { recursive: true });
  await fs.writeFile(path.join(root, 'Shop.h'), '#pragma once\nint debit(int amount);\n');
  await fs.writeFile(path.join(root, 'src', 'Shop.cpp'), '#include "../Shop.h"\nint debit(int amount) {\n  return amount - 1; // off by one\n}\n');
  await fs.writeFile(path.join(root, 'src', 'deep', 'Refund.cpp'), 'int refund() { return debit(0); }\n');
  await fs.writeFile(path.join(root, 'node_modules', 'pkg', 'index.js'), 'debit');
  await fs.writeFile(path.join(root, 'blob.bin'), Buffer.from([0, 1, 2, 100, 101, 98, 105, 116]));
  await fs.writeFile(path.join(base, 'secret.cpp'), 'int debit() { return 42; }');
  return { base, root };
}

test('tool definitions describe listing, searching and reading', () => {
  assert.deepEqual(definitions.map(tool => tool.name), ['zucchini_list_files', 'zucchini_search', 'zucchini_read_file']);
  assert.ok(definitions.every(tool => tool.inputSchema.type === 'object' && tool.description));
});

test('lists files with wildcards and recursion, skipping dependency folders', async t => {
  const { root } = await workspace(t);
  const tools = createSourceTools([root]);
  const top = await tools.invoke('zucchini_list_files', {});
  assert.match(top, /Shop\.h/);
  assert.match(top, new RegExp(`src\\${path.sep}$`, 'm'));
  assert.doesNotMatch(top, /Refund\.cpp/);
  const sources = await tools.invoke('zucchini_list_files', { path: 'src', recursive: true, pattern: '*.cpp' });
  assert.deepEqual(sources.split('\n').sort(), [path.join(root, 'src', 'Shop.cpp'), path.join(root, 'src', 'deep', 'Refund.cpp')].sort());
  assert.doesNotMatch(await tools.invoke('zucchini_list_files', { recursive: true }), /node_modules/);
});

test('searches text files by literal or regular expression and skips binaries', async t => {
  const { root } = await workspace(t);
  const tools = createSourceTools([root]);
  const literal = await tools.invoke('zucchini_search', { query: 'debit(' });
  assert.match(literal, /Shop\.h:2: int debit\(int amount\);/);
  assert.match(literal, /Shop\.cpp:2:/);
  assert.match(literal, /Refund\.cpp:1:/);
  assert.doesNotMatch(literal, /node_modules|blob\.bin|secret/);
  const regex = await tools.invoke('zucchini_search', { query: 'return \\w+ - 1', regex: true, include: '*.cpp' });
  assert.equal(regex, `${path.join(root, 'src', 'Shop.cpp')}:3: return amount - 1; // off by one`);
  assert.equal(await tools.invoke('zucchini_search', { query: 'DEBIT', path: 'src', ignoreCase: true, maxResults: 1 }).then(text => text.split('\n').length), 2);
  assert.match(await tools.invoke('zucchini_search', { query: '(', regex: true }), /^Error: /);
  assert.equal(await tools.invoke('zucchini_search', { query: 'nothing matches this' }), 'No matches.');
});

test('reads line ranges, reports unsaved editor changes and refuses paths outside the workspace', async t => {
  const { base, root } = await workspace(t);
  const dirty = path.join(root, 'src', 'Shop.cpp');
  const tools = createSourceTools([root], { isDirty: file => file === dirty });
  const text = await tools.invoke('zucchini_read_file', { path: 'src/Shop.cpp', startLine: 2, endLine: 3 });
  assert.match(text, /\(lines 2-3 of 5\)/);
  assert.match(text, /^3:   return amount - 1;/m);
  assert.doesNotMatch(text, /^1:/m);
  assert.match(text, /unsaved editor changes/);
  assert.doesNotMatch(await tools.invoke('zucchini_read_file', { path: 'Shop.h' }), /unsaved/);
  assert.match(await tools.invoke('zucchini_read_file', { path: path.join(base, 'secret.cpp') }), /^Error: Path is outside the workspace/);
  assert.match(await tools.invoke('zucchini_read_file', { path: '../secret.cpp' }), /^Error: /);
  assert.match(await tools.invoke('zucchini_read_file', { path: 'blob.bin' }), /^Error: Not a readable text file/);
  assert.match(await tools.invoke('zucchini_read_file', { path: 'missing.cpp' }), /^Error: Not found/);
  assert.match(await tools.invoke('zucchini_unknown', {}), /unknown tool/);
});

test('symbolic links cannot escape the workspace', { skip: process.platform === 'win32' }, async t => {
  const { base, root } = await workspace(t);
  await fs.symlink(path.join(base, 'secret.cpp'), path.join(root, 'link.cpp'));
  const tools = createSourceTools([root]);
  assert.match(await tools.invoke('zucchini_read_file', { path: 'link.cpp' }), /^Error: Path is outside the workspace/);
});

test('long files are truncated with a continuation hint', async t => {
  const { root } = await workspace(t);
  await fs.writeFile(path.join(root, 'Long.cpp'), Array.from({ length: 5000 }, (_, i) => `// line ${i} ${'x'.repeat(40)}`).join('\n'));
  const text = await createSourceTools([root]).invoke('zucchini_read_file', { path: 'Long.cpp' });
  assert.ok(text.length < 70000);
  assert.match(text, /Truncated; continue with startLine \d+\./);
});

test('companion implementation files are found next to headers', async t => {
  const { root } = await workspace(t);
  await fs.writeFile(path.join(root, 'Wallet.h'), '');
  await fs.writeFile(path.join(root, 'Wallet.cpp'), '');
  await fs.writeFile(path.join(root, 'Wallet.inl'), '');
  assert.deepEqual(await companionFiles([path.join(root, 'Shop.h'), path.join(root, 'Wallet.h'), path.join(root, 'Wallet.inl')]),
    [path.join(root, 'Wallet.cpp')]);
});
