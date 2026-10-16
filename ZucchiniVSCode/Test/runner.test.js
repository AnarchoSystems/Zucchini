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

const path = require('node:path');
const { validateGeneration } = require('../src/runner');
function validationSetup({ buildCode = 0, testCode = 0, accepted = true, scenarios = true, discoveryCode = 0 } = {}) {
  const file = path.resolve('project with spaces', 'Shop.feature');
  const calls = [];
  const scenario = { name: 'new', test_name: 'new', uri: file, line: 5, accepted };
  const options = {
    directory: path.resolve('build with spaces'), configuration: 'Debug', ctest: 'custom-ctest', fixture: 'Shop',
    changes: [{ file, before: 'Feature: shop\r\n  Scenario: old\r\n    Then ok\r\n' }],
    token: { isCancellationRequested: false },
    readReport: async () => ({ discovery_success: accepted, scenarios: scenarios ? [
      { ...scenario, name: 'old', test_name: 'old', line: 2 }, scenario
    ] : [] }),
    async execute(executable, args, onOutput) {
      if (args.includes('--show-only=json-v1')) assert.equal(onOutput, undefined, 'Discovery JSON must not be streamed to the output channel');
      calls.push({ executable, args });
      if (executable === 'cmake') return { code: buildCode, output: 'build diagnostic' };
      if (args.includes('--show-only=json-v1')) return { code: discoveryCode, output: discoveryCode ? 'discovery error' : JSON.stringify({ tests: [
        { name: 'Shop.new[1]', command: ['Shop', '--gtest_filter=Shop.new'] }
      ] }) };
      return { code: testCode, output: 'test diagnostic' };
    }
  };
  return { options, calls, run: () => validateGeneration(options) };
}

test('generation builds with configuration and exercises only appended scenarios with exact CTest selection', async () => {
  const state = validationSetup();
  const result = await state.run();
  assert.equal(result.ready, true);
  assert.deepEqual(result.outcomes, ['passed']);
  assert.deepEqual(state.calls[0], { executable: 'cmake', args: ['--build', state.options.directory, '--parallel', '--config', 'Debug'] });
  assert.deepEqual(state.calls[2], { executable: 'custom-ctest', args: ['--test-dir', state.options.directory, '-C', 'Debug', '-R', '^Shop\\.new\\[1\\]$', '--no-tests=error', '--output-on-failure'] });
  assert.match(result.diagnostics, /test diagnostic/);
});

test('generation distinguishes test failures from build, discovery, and execution errors', async () => {
  const failure = await validationSetup({ testCode: 8 }).run();
  assert.equal(failure.ready, true);
  assert.deepEqual(failure.outcomes, ['failed']);
  for (const options of [{ buildCode: 1 }, { discoveryCode: 1 }, { testCode: 1 }, { accepted: false }, { scenarios: false }]) {
    const state = validationSetup(options);
    assert.equal((await state.run()).ready, false);
    if (options.buildCode) assert.equal(state.calls.length, 1);
    if (options.accepted === false || options.scenarios === false) assert.equal(state.calls.length, 2);
  }
});

test('generation cancellation after build prevents discovery and test execution', async () => {
  const state = validationSetup();
  const execute = state.options.execute;
  state.options.execute = async (...args) => {
    const result = await execute(...args);
    state.options.token.isCancellationRequested = true;
    return result;
  };
  assert.equal(await state.run(), undefined);
  assert.equal(state.calls.length, 1);
});
