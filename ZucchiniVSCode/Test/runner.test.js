'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { buildTarget, buildConfiguration, selectTest, exactRegex, processOutput } = require('../src/runner');
test('build configuration follows multi-config report directories unless overridden', () => {
  const path = require('node:path');
  assert.equal(buildConfiguration(path.join('build with spaces', 'Debug', 'rules.json')), 'Debug');
  assert.equal(buildConfiguration(path.join('build', 'relwithdebinfo', 'rules.json')), 'relwithdebinfo');
  assert.equal(buildConfiguration(path.join('build', 'Shop', 'rules.json')), '');
  assert.equal(buildConfiguration(path.join('build', 'Debug', 'rules.json'), 'Release'), 'Release');
});
test('generation targets the executable rather than the fixture, with portable names and overrides', () => {
  assert.equal(buildTarget('ShopTests'), 'ShopTests');
  assert.equal(buildTarget('C:\\build with spaces\\ShopTests.EXE'), 'ShopTests');
  assert.equal(buildTarget('/build with spaces/ShopTests'), 'ShopTests');
  assert.equal(buildTarget('ShopTests-d.exe', 'ShopTarget'), 'ShopTarget');
  assert.equal(buildTarget(undefined, 'LegacyTarget'), 'LegacyTarget');
  for (const target of [undefined, '', ' ', '-bad', 'one;two', 'one\ntwo']) {
    assert.throws(() => buildTarget(undefined, target), /Set zucchini\.fixtures\.<fixture>\.buildTarget/);
  }
});
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
    directory: path.resolve('build with spaces'), configuration: 'Debug', target: 'ShopTests', ctest: 'custom-ctest', fixture: 'Shop',
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
      const junit = args[args.indexOf('--output-junit') + 1];
      await require('node:fs/promises').writeFile(junit, `<testsuite><testcase name="Shop.new[1]" status="${testCode ? 'fail' : 'run'}"></testcase></testsuite>`);
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
  assert.deepEqual(state.calls[0], { executable: 'cmake', args: ['--build', state.options.directory, '--target', 'ShopTests', '--parallel', '--config', 'Debug'] });
  assert.deepEqual(state.calls[2], { executable: 'custom-ctest', args: ['--test-dir', state.options.directory, '-C', 'Debug', `-j${require('../src/runner').parallelJobs()}`, '-R', '^Shop\\.new\\[1\\]$', '--no-tests=error', '--output-on-failure', '--output-junit', state.calls[2].args.at(-1)] });
  assert.match(result.diagnostics, /test diagnostic/);
});

test('generation never falls back to the default build target', async () => {
  const state = validationSetup();
  delete state.options.target;
  await assert.rejects(state.run(), /buildTarget/);
  assert.deepEqual(state.calls, []);
});

test('generation targets single-configuration builds without adding a configuration', async () => {
  const state = validationSetup();
  delete state.options.configuration;
  await state.run();
  assert.deepEqual(state.calls[0].args, ['--build', state.options.directory, '--target', 'ShopTests', '--parallel']);
  assert.ok(!state.calls[1].args.includes('-C'));
});

test('real CMake validation builds only the fixture target', async t => {
  const { spawnSync } = require('node:child_process');
  if (spawnSync('cmake', ['--version']).status !== 0) return t.skip('CMake is unavailable');
  const fs = require('node:fs/promises');
  const os = require('node:os');
  const root = await fs.mkdtemp(path.join(os.tmpdir(), 'zucchini scoped build '));
  t.after(() => fs.rm(root, { recursive: true, force: true }));
  const directory = path.join(root, 'build with spaces');
  // Building the default target would fail on Unrelated; incrementality is CMake's concern.
  await fs.writeFile(path.join(root, 'CMakeLists.txt'), `cmake_minimum_required(VERSION 3.20)
project(ScopedBuild NONE)
add_custom_target(ShopTests ALL COMMAND "\${CMAKE_COMMAND}" -E touch "\${CMAKE_CURRENT_BINARY_DIR}/output.txt" VERBATIM)
add_custom_target(Unrelated ALL COMMAND "\${CMAKE_COMMAND}" -E false VERBATIM)
`);
  const configure = await processOutput('cmake', ['-S', root, '-B', directory]);
  assert.equal(configure.code, 0, configure.output);
  const state = validationSetup();
  state.options.directory = directory;
  state.options.token.onCancellationRequested = () => ({ dispose() {} });
  state.options.readReport = async () => ({ scenarios: [] });
  state.options.execute = async (executable, args, log, token) => executable === 'cmake'
    ? processOutput(executable, args, log, token)
    : { code: 0, output: '{"tests":[]}' };
  const result = await state.run();
  assert.doesNotMatch(result.diagnostics, /Build failed/);
  await fs.access(path.join(directory, 'output.txt'));
});

test('generation distinguishes test failures from build, discovery, and execution errors', async () => {
  const failure = await validationSetup({ testCode: 8 }).run();
  assert.equal(failure.ready, true);
  assert.deepEqual(failure.outcomes, ['failed']);
  assert.deepEqual(failure.results.map(({ scenario, outcome }) => [scenario.name, outcome]), [['new', 'failed']], 'Outcomes are paired with new scenarios only');
  assert.ok(Array.isArray(failure.report.scenarios));
  assert.deepEqual((await validationSetup({ accepted: false }).run()).results.map(({ outcome }) => outcome), ['rejected']);
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


test('parallel settings default to available processors with explicit overrides and a sequential switch', () => {
  const { parallelJobs } = require('../src/runner');
  const os = require('node:os');
  assert.equal(parallelJobs(), os.availableParallelism?.() || os.cpus().length || 1);
  assert.equal(parallelJobs({ jobs: 4 }), 4);
  assert.equal(parallelJobs({ parallel: false, jobs: 4 }), 1);
  assert.equal(parallelJobs({ jobs: 0 }), parallelJobs());
});

test('JUnit outcomes preserve mixed passes, failures, skips and escaped names', () => {
  const { junitStatuses } = require('../src/runner');
  assert.deepEqual([...junitStatuses(`<testsuite>
    <testcase name="pass &amp; &quot;quoted&quot;" status="run"><system-out>&lt;failure/&gt;</system-out></testcase>
    <testcase name="fail" status="fail"><failure message="Failed"/></testcase>
    <testcase name="skip" status="notrun"><skipped/></testcase>
  </testsuite>`)], [['pass & "quoted"', 'passed'], ['fail', 'failed'], ['skip', 'skipped']]);
});

test('selected tests share one exact filter and one CTest scheduler', async () => {
  const { runTests } = require('../src/runner');
  const fs = require('node:fs/promises');
  const calls = [];
  const tests = [{ name: 'Shop.A+B[0]' }, { name: 'Shop.Other' }];
  const result = await runTests({ ctest: 'ctest', base: ['--test-dir', 'build with spaces', '-C', 'Debug'], tests, jobs: 4,
    token: { isCancellationRequested: false },
    async execute(executable, args) {
      calls.push(args);
      const selection = new RegExp(args[args.indexOf('-R') + 1]);
      for (const test of tests) assert.ok(selection.test(test.name));
      assert.ok(!selection.test('Shop.OtherExtra'));
      assert.ok(!selection.test('prefixShop.Other'));
      await fs.writeFile(args.at(-1), '<testsuite><testcase name="Shop.A+B[0]" status="run"/><testcase name="Shop.Other" status="fail"><failure/></testcase></testsuite>');
      return { code: 8, output: 'mixed results' };
    }
  });
  assert.equal(calls.length, 1);
  assert.ok(calls[0].includes('-j4'));
  assert.deepEqual(result.outcomes, ['passed', 'failed']);
  await assert.rejects(fs.access(calls[0].at(-1)), /ENOENT/);
});

test('real CTest runs the filtered selection concurrently and reports each outcome', async t => {
  const { spawnSync } = require('node:child_process');
  if (spawnSync('ctest', ['--version']).status !== 0) return t.skip('CTest is unavailable');
  const fs = require('node:fs/promises');
  const os = require('node:os');
  const { runTests } = require('../src/runner');
  const directory = await fs.mkdtemp(path.join(os.tmpdir(), 'zucchini parallel test '));
  t.after(() => fs.rm(directory, { recursive: true, force: true }));
  const script = path.join(directory, 'overlap.js');
  await fs.writeFile(script, `
    const fs = require('node:fs');
    const path = require('node:path');
    const [directory, id, other] = process.argv.slice(2);
    fs.writeFileSync(path.join(directory, id), 'started');
    const deadline = Date.now() + 5000;
    const timer = setInterval(() => {
      if (fs.existsSync(path.join(directory, other))) { clearInterval(timer); process.exit(id === 'one' ? 0 : 1); }
      if (Date.now() > deadline) { clearInterval(timer); process.exit(2); }
    }, 20);
  `);
  const quote = value => `[=[${value.replaceAll('\\', '/')}]=]`;
  await fs.writeFile(path.join(directory, 'CTestTestfile.cmake'), [
    `add_test("first & test" ${quote(process.execPath)} ${quote(script)} ${quote(directory)} "one" "two")`,
    `add_test("second[1]" ${quote(process.execPath)} ${quote(script)} ${quote(directory)} "two" "one")`,
    `add_test("unselected" "missing-executable-that-must-not-run")`
  ].join('\n'));
  const result = await runTests({ ctest: 'ctest', base: ['--test-dir', directory],
    tests: [{ name: 'first & test' }, { name: 'second[1]' }], reportDirectory: path.join(directory, 'dashboard'), jobs: 2, token: { isCancellationRequested: false, onCancellationRequested() { return { dispose() {} }; } } });
  assert.equal(result.code, 8, result.output);
  assert.deepEqual(result.outcomes, ['passed', 'failed'], result.output);
  assert.doesNotMatch(result.output, /unselected/);
  const exported = await fs.readFile(path.join(directory, 'dashboard', 'ctest-results.xml'), 'utf8');
  assert.deepEqual([...require('../src/runner').junitStatuses(exported).values()], ['passed', 'failed']);
  assert.match(await fs.readFile(path.join(directory, 'dashboard', 'ctest-results.js'), 'utf8'), /^window\.zucchiniCTest\(/);
});

test('local runs publish JUnit and executable dashboard data, replace previous runs, and preserve them on cancellation', async t => {
  const { runTests } = require('../src/runner');
  const fs = require('node:fs/promises');
  const vm = require('node:vm');
  const root = await fs.mkdtemp(path.join(require('node:os').tmpdir(), 'zucchini dashboard '));
  t.after(() => fs.rm(root, { recursive: true, force: true }));
  const reportDirectory = path.join(root, 'reports with spaces', 'Debug');
  const token = { isCancellationRequested: false };
  let xml, code = 0;
  const options = { ctest: 'ctest', base: ['--test-dir', root, '-C', 'Debug'],
    tests: [{ name: 'Shop.café & tea/café & tea' }], reportDirectory, token,
    async execute(executable, args) {
      await fs.writeFile(args[args.indexOf('--output-junit') + 1], xml);
      return { code, output: 'test output' };
    }
  };
  for (const status of ['run', 'fail']) {
    code = status === 'fail' ? 8 : 0;
    xml = `<testsuite>\r\n<testcase name="Shop.café &amp; tea/café &amp; tea" status="${status}"/>\r\n</testsuite>`;
    const result = await runTests(options);
    assert.equal(result.code, code);
    assert.deepEqual(result.outcomes, [status === 'fail' ? 'failed' : 'passed']);
    assert.equal(await fs.readFile(path.join(reportDirectory, 'ctest-results.xml'), 'utf8'), xml);
    let imported;
    vm.runInNewContext(await fs.readFile(path.join(reportDirectory, 'ctest-results.js'), 'utf8'), {
      window: { zucchiniCTest(value) { imported = value; } }
    });
    assert.equal(imported, xml, 'Dashboard callback must receive the exact CTest XML');
  }
  const previous = await fs.readFile(path.join(reportDirectory, 'ctest-results.js'), 'utf8');
  options.execute = async () => { token.isCancellationRequested = true; return { code: null, output: 'cancelled' }; };
  assert.equal(await runTests(options), undefined);
  assert.equal(await fs.readFile(path.join(reportDirectory, 'ctest-results.js'), 'utf8'), previous);
  assert.deepEqual((await fs.readdir(reportDirectory)).sort(), ['ctest-results.js', 'ctest-results.xml']);
});

test('generation validation publishes results beside its fixture report', async t => {
  const fs = require('node:fs/promises');
  const reportDirectory = await fs.mkdtemp(path.join(require('node:os').tmpdir(), 'zucchini validation report '));
  t.after(() => fs.rm(reportDirectory, { recursive: true, force: true }));
  const state = validationSetup({ testCode: 8 });
  state.options.reportDirectory = reportDirectory;
  assert.equal((await state.run()).ready, true);
  assert.match(await fs.readFile(path.join(reportDirectory, 'ctest-results.xml'), 'utf8'), /status="fail"/);
  assert.match(await fs.readFile(path.join(reportDirectory, 'ctest-results.js'), 'utf8'), /^window\.zucchiniCTest\(/);
});
