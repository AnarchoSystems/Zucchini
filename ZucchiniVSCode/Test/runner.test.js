'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { selectTest, exactRegex, processOutput } = require('../src/runner');
test('CTest matching uses exact gtest filter and source paths with Windows drive letters', () => {
  const scenario = { test_name: 'S', uri: 'C:\\project with spaces\\Shop.feature', line: 12 };
  const command = ['C:/build/Shop.exe', '--gtest_filter=Shop.S'];
  const tests = [1, 12].map(line => ({ name: `test-${line}`, command, properties: [{ name: 'DEF_SOURCE_LINE', value: `C:/project with spaces/Shop.feature:${line}` }] }));
  assert.equal(selectTest(tests, 'Shop', scenario).name, 'test-12');
  assert.throws(() => selectTest(tests.map(t => ({ ...t, properties: [] })), 'Shop', scenario), /Expected one/);
});
test('CTest regex anchors special characters', () => {
  const value = 'Shop.A+B[0]/a.b(foo)\\bar$';
  const regex = new RegExp(exactRegex(value));
  assert.ok(regex.test(value));
  assert.ok(!regex.test('prefix' + value));
  assert.ok(!regex.test(value + 'suffix'));
});
test('process arguments preserve spaces and shell metacharacters', async () => {
  const value = 'spaces $HOME ; echo nope';
  const result = await processOutput(process.execPath, ['-e', 'require("node:fs").writeSync(1, process.argv[1])', value]);
  assert.equal(result.code, 0);
  assert.equal(result.output, value);
});
test('executable disambiguates identical fixtures and feature sources', () => {
  const scenario = { test_name: 'S', uri: 'C:/src/shared.feature', line: 1 };
  const tests = ['One.exe', 'Two.exe'].map(executable => ({ name: executable,
    command: [`C:/build with spaces/${executable}`, '--gtest_filter=RawFixture.S'] }));
  assert.equal(selectTest(tests, 'RawFixture', scenario, 'Two.exe').name, 'Two.exe');
  assert.throws(() => selectTest(tests, 'RawFixture', scenario, 'Missing.exe'), /Expected one/);
});
