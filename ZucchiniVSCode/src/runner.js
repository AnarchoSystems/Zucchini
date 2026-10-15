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

function selectTest(tests, fixture, scenario) {
  const filter = `--gtest_filter=${fixture}.${scenario.test_name}`;
  const candidates = tests.filter(t => t.command?.includes(filter));
  // A build can register the same fixture name more than once. Require source identity.
  const matching = candidates.filter(t => t.properties?.some(p => p.name === 'DEF_SOURCE_LINE' &&
    String(p.value).replaceAll('\\', '/') === `${scenario.uri}:${scenario.line}`.replaceAll('\\', '/')));
  if (matching.length === 1) return matching[0];
  if (candidates.length === 1) return candidates[0];
  throw new Error(`Expected one registered CTest test for ${fixture}.${scenario.test_name}; found ${candidates.length}. Refresh/rebuild manually or configure a narrower buildDirectory.`);
}

function exactRegex(text) { return '^' + text.replace(/[.*+?^${}()|[\]\\]/g, '\\$&') + '$'; }
module.exports = { buildDirectory, processOutput, selectTest, exactRegex };
