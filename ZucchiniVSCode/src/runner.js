'use strict';
const { spawn } = require('node:child_process');
const fs = require('node:fs/promises');
const path = require('node:path');

async function buildDirectory(reportPath, override) {
  if (override) return override;
  let dir = path.dirname(reportPath);
  while (true) {
    try { await fs.access(path.join(dir, 'CMakeCache.txt')); return dir; } catch { /* ascend */ }
    const parent = path.dirname(dir);
    if (parent === dir) throw new Error('Cannot locate CMakeCache.txt. Set zucchini.fixtures.<fixture>.buildDirectory.');
    dir = parent;
  }
}

function processOutput(executable, args, onOutput = () => {}, token) {
  return new Promise((resolve, reject) => {
    if (token?.isCancellationRequested) return reject(new Error('Cancelled'));
    const child = spawn(executable, args, { shell: false, windowsHide: true });
    let output = '';
    const subscription = token?.onCancellationRequested(() => child.kill());
    for (const stream of [child.stdout, child.stderr]) stream.on('data', data => { output += data.toString(); onOutput(data.toString()); });
    child.on('error', error => { subscription?.dispose(); reject(error); });
    child.on('close', code => { subscription?.dispose(); resolve({ code, output }); });
  });
}

function selectTest(tests, fixture, scenario, executable) {
  const filter = `--gtest_filter=${fixture}.${scenario.test_name}`;
  const candidates = tests.filter(t => t.command?.includes(filter) &&
    (!executable || path.win32.basename(t.command[0]) === executable));
  // A build can register the same fixture name more than once. Require source identity.
  const matching = candidates.filter(t => t.properties?.some(p => p.name === 'DEF_SOURCE_LINE' &&
    String(p.value).replaceAll('\\', '/') === `${scenario.uri}:${scenario.line}`.replaceAll('\\', '/')));
  if (matching.length === 1) return matching[0];
  if (candidates.length === 1) return candidates[0];
  throw new Error(`Expected one registered CTest test for ${fixture}.${scenario.test_name}; found ${candidates.length}. Refresh/rebuild manually or configure a narrower buildDirectory.`);
}

async function validateGeneration({ directory, configuration, ctest, fixture, changes, readReport, execute = processOutput, log, token }) {
  const base = ['--test-dir', directory, ...(configuration ? ['-C', configuration] : [])];
  const build = await execute('cmake', ['--build', directory, '--parallel', ...(configuration ? ['--config', configuration] : [])], log, token);
  if (token.isCancellationRequested) return;
  if (build.code !== 0) return { ready: false, diagnostics: `Build failed (${build.code}):\n${build.output}` };
  const discovery = await execute(ctest, [...base, '--show-only=json-v1'], undefined, token);
  if (token.isCancellationRequested) return;
  if (discovery.code !== 0) return { ready: false, diagnostics: `CTest discovery failed (${discovery.code}):\n${discovery.output}` };
  const report = await readReport();
  const scenarios = report.scenarios.filter(scenario => changes.some(change =>
    path.resolve(change.file) === path.resolve(scenario.uri) && scenario.line > (change.before ? change.before.split(/\r?\n/).length : 0)));
  const diagnostics = [];
  let ready = report.discovery_success !== false && scenarios.length > 0;
  const outcomes = [];
  const tests = JSON.parse(discovery.output).tests;
  for (const scenario of scenarios) {
    if (token.isCancellationRequested) return;
    if (scenario.accepted === false) { ready = false; diagnostics.push(`${scenario.name}: rejected during discovery`); continue; }
    let test;
    try { test = selectTest(tests, fixture, scenario, report.executable); }
    catch (error) { ready = false; diagnostics.push(error.message); continue; }
    const result = await execute(ctest, [...base, '-R', exactRegex(test.name), '--no-tests=error', '--output-on-failure'], log, token);
    // CTest uses 8 for test failures. Other errors are not evidence of an intended failure.
    if (result.code !== 0 && result.code !== 8) ready = false;
    outcomes.push(result.code === 0 ? 'passed' : 'failed');
    diagnostics.push(`${scenario.name}: exit ${result.code}\n${result.output}`);
  }
  return { ready, outcomes, diagnostics: `Discovery successful: ${report.discovery_success}; new scenarios: ${scenarios.length}\n${diagnostics.join('\n')}` };
}

function exactRegex(text) { return '^' + text.replace(/[.*+?^${}()|[\]\\]/g, '\\$&') + '$'; }
module.exports = { buildDirectory, processOutput, selectTest, exactRegex, validateGeneration };
