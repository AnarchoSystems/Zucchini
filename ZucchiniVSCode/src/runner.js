'use strict';
const { spawn } = require('node:child_process');
const fs = require('node:fs/promises');
const path = require('node:path');
const os = require('node:os');
const { randomUUID } = require('node:crypto');

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

// availableParallelism respects the host's CPU affinity and works on Windows too.
function parallelJobs({ parallel = true, jobs = 0 } = {}) {
  if (!parallel) return 1;
  return Number.isInteger(jobs) && jobs > 0 ? jobs : Math.max(1, os.availableParallelism?.() || os.cpus().length || 1);
}

function xmlText(value) {
  return value.replace(/&(#x[0-9a-f]+|#\d+|amp|lt|gt|quot|apos);/gi, (_, entity) => {
    if (entity.startsWith('#')) return String.fromCodePoint(entity[1].toLowerCase() === 'x' ? parseInt(entity.slice(2), 16) : Number(entity.slice(1)));
    return { amp: '&', lt: '<', gt: '>', quot: '"', apos: "'" }[entity];
  });
}

// Read CTest's JUnit testcase headers; output text is XML-escaped by CTest.
function junitStatuses(xml) {
  const statuses = new Map();
  for (const match of xml.matchAll(/<testcase\b([^>]*?)(?:\/>|>([\s\S]*?)<\/testcase>)/g)) {
    const attributes = Object.fromEntries([...match[1].matchAll(/([\w-]+)="([^"]*)"/g)].map(m => [m[1], xmlText(m[2])]));
    if (!attributes.name || statuses.has(attributes.name)) throw new Error('Invalid or duplicate CTest JUnit test name.');
    const body = match[2] || '';
    const status = /<(?:failure|error)\b/.test(body) || attributes.status === 'fail' ? 'failed'
      : /<skipped\b/.test(body) || ['notrun', 'disabled'].includes(attributes.status) ? 'skipped'
        : attributes.status === 'run' ? 'passed' : 'incomplete';
    statuses.set(attributes.name, status);
  }
  return statuses;
}

async function publishCTestResults(directory, xml, report, tests) {
  await fs.mkdir(directory, { recursive: true });
  const files = [
    ['ctest-results.xml', xml],
    ['ctest-results.js', `window.zucchiniCTest(${JSON.stringify(xml)});\n`]
  ];
  if (report) {
    const names = {};
    for (const test of tests) {
      const filter = test.command?.find(arg => arg.startsWith(`--gtest_filter=${report.fixture}.`));
      if (filter) names[test.name] = filter.slice(`--gtest_filter=${report.fixture}.`.length);
    }
    files.push(['ctest-results.json', JSON.stringify({
      fixture: report.fixture, discovery_id: report.discovery_id, names, xml
    })]);
  }
  for (const [name, content] of files) {
    const destination = path.join(directory, name);
    const temporary = destination + '.tmp-' + randomUUID();
    try {
      await fs.writeFile(temporary, content, 'utf8');
      await fs.rename(temporary, destination);
    } finally { await fs.rm(temporary, { force: true }); }
  }
}

async function runTests({ ctest, base, tests, reportDirectory, report, jobs = parallelJobs(), execute = processOutput, log, token }) {
  if (!tests.length) throw new Error('No CTest tests selected.');
  if (token.isCancellationRequested) return;
  const temporary = await fs.mkdtemp(path.join(os.tmpdir(), 'zucchini-ctest-'));
  const junit = path.join(temporary, 'results.xml');
  try {
    // One CTest scheduler preserves resource locks, dependencies and PROCESSORS.
    const selection = tests.map(test => exactRegex(test.name)).join('|');
    const result = await execute(ctest, [...base, `-j${jobs}`, '-R', selection, '--no-tests=error', '--output-on-failure', '--output-junit', junit], log, token);
    if (token.isCancellationRequested) return;
    let statuses, xml;
    try {
      xml = await fs.readFile(junit, 'utf8');
      statuses = junitStatuses(xml);
    } catch (error) { throw new Error(`Cannot read CTest results: ${error.message}\n${result.output}`); }
    if (reportDirectory) await publishCTestResults(reportDirectory, xml, report, tests);
    return { ...result, outcomes: tests.map(test => statuses.get(test.name) || 'incomplete') };
  } finally { await fs.rm(temporary, { recursive: true, force: true }); }
}

async function validateGeneration({ directory, configuration, ctest, fixture, reportDirectory, changes, readReport, execute = processOutput, log, token, jobs = parallelJobs() }) {
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
  const selected = [];
  const tests = JSON.parse(discovery.output).tests;
  for (const scenario of scenarios) {
    if (token.isCancellationRequested) return;
    if (scenario.accepted === false) { ready = false; diagnostics.push(`${scenario.name}: rejected during discovery`); continue; }
    let test;
    try { test = selectTest(tests, fixture, scenario, report.executable); }
    catch (error) { ready = false; diagnostics.push(error.message); continue; }
    selected.push(test);
  }
  let outcomes = [];
  if (selected.length) {
    const result = await runTests({ ctest, base, tests: selected, reportDirectory, report, jobs, execute, log, token });
    if (!result || token.isCancellationRequested) return;
    outcomes = result.outcomes;
    // Infrastructure errors, skipped or missing tests cannot satisfy generation.
    if ((result.code !== 0 && result.code !== 8) || outcomes.some(outcome => !['passed', 'failed'].includes(outcome))) ready = false;
    diagnostics.push(`CTest exit ${result.code}\n${selected.map((test, i) => `${test.name}: ${outcomes[i]}`).join('\n')}\n${result.output}`);
  }
  return { ready, outcomes, diagnostics: `Discovery successful: ${report.discovery_success}; new scenarios: ${scenarios.length}\n${diagnostics.join('\n')}` };
}

function exactRegex(text) { return '^' + text.replace(/[.*+?^${}()|[\]\\]/g, '\\$&') + '$'; }
module.exports = { buildDirectory, processOutput, selectTest, exactRegex, validateGeneration, parallelJobs, runTests, junitStatuses };
