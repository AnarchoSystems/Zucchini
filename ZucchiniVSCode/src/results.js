'use strict';
const fs = require('node:fs/promises');
const path = require('node:path');
const { junitStatuses } = require('./runner');

async function loadResults(fixture) {
  const results = new Map();
  try {
    const saved = JSON.parse(await fs.readFile(path.join(path.dirname(fixture.path), 'ctest-results.json'), 'utf8'));
    if (saved.fixture !== fixture.report.fixture) return results;
    const statuses = junitStatuses(saved.xml);
    const testNames = new Map(Object.entries(saved.names).map(([name, scenario]) => [scenario, name]));
    for (const scenario of fixture.report.scenarios) {
      const name = testNames.get(scenario.test_name);
      if (name) results.set(scenario.id, {
        fixture: saved.fixture, test_name: scenario.test_name,
        discovery_id: saved.discovery_id, status: statuses.get(name) || 'incomplete'
      });
    }
    return results;
  } catch (error) {
    // Only older reports without a CTest snapshot use legacy per-scenario files.
    if (error.code !== 'ENOENT') return results;
  }
  for (const scenario of fixture.report.scenarios) {
    if (!/^[a-f0-9]{16}$/.test(scenario.result_key || '')) continue;
    try {
      const result = JSON.parse(await fs.readFile(path.join(path.dirname(fixture.path), 'results', scenario.result_key + '.json'), 'utf8'));
      if (result.fixture === fixture.report.fixture && result.test_name === scenario.test_name) results.set(scenario.id, result);
    } catch { /* A missing or temporarily replaced result is unknown, never passing. */ }
  }
  return results;
}

function scenarioStatus(fixture, scenario, attempt) {
  if (scenario.sourceState === 'pending') return 'pending';
  const local = attempt?.discovery_id === fixture.report.discovery_id ? attempt : undefined;
  const persisted = fixture.results?.get(scenario.id);
  const result = local && (['running', 'cancelled', 'incomplete'].includes(local.status) ||
    persisted?.discovery_id !== fixture.report.discovery_id) ? local : persisted || local;
  if (result && result.status !== 'not_run') {
    if (!fixture.report.discovery_id || result.discovery_id !== fixture.report.discovery_id ||
        (scenario.sourceState && scenario.sourceState !== 'current')) return 'stale';
    return ['passed', 'failed', 'skipped', 'cancelled', 'running'].includes(result.status) ? result.status : ['started', 'incomplete'].includes(result.status) ? 'incomplete' : 'not_run';
  }
  if (scenario.sourceState && scenario.sourceState !== 'current') return 'stale';
  return scenario.accepted === false ? 'rejected' : 'not_run';
}

function aggregateStatus(statuses) {
  // Show only actual current executions. A discovery rejection is not a failed test.
  if (statuses.includes('failed')) return 'failed';
  if (statuses.includes('running')) return 'running';
  if (statuses.includes('passed')) return 'passed';
  if (statuses.includes('stale')) return 'stale';
  return 'not_run';
}
module.exports = { loadResults, scenarioStatus, aggregateStatus };
