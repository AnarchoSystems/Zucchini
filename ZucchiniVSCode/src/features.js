'use strict';
const fs = require('node:fs/promises');
const path = require('node:path');
let parserModules;
async function descriptors(text) {
  parserModules ||= Promise.all([import('@cucumber/gherkin'), import('@cucumber/messages')]);
  const [{ Parser, AstBuilder, GherkinClassicTokenMatcher }, { IdGenerator }] = await parserModules;
  const document = new Parser(new AstBuilder(IdGenerator.incrementing()), new GherkinClassicTokenMatcher()).parse(text);
  const feature = document.feature;
  if (!feature) return [];
  function clean(value) {
    if (Array.isArray(value)) return value.map(clean);
    if (value && typeof value === 'object') return Object.fromEntries(Object.entries(value).filter(([key]) => !['id', 'location', 'children'].includes(key)).map(([key, value]) => [key, clean(value)]));
    return value;
  }
  const found = [];
  function walk(children, scopes, rule = '') {
    const background = children.find(child => child.background)?.background;
    const scope = [...scopes, ...(background ? [background] : [])];
    for (const child of children) {
      if (child.rule) walk(child.rule.children, [...scope, child.rule], child.rule.name);
      if (child.scenario) {
        const nodes = [...scope, child.scenario];
        found.push({ node: child.scenario, nodes, name: child.scenario.name, feature: feature.name, rule, line: child.scenario.location.line,
          signature: JSON.stringify(clean(nodes)) });
      }
    }
  }
  walk(feature.children, [feature]);
  return found;
}

function mapLocations(old, current, locations) {
  if (!old || !current || typeof old !== 'object') return;
  if (old.location && current.location) locations.set(old.location.line, current.location.line);
  if (Array.isArray(old)) old.forEach((value, i) => mapLocations(value, current[i], locations));
  else for (const key of Object.keys(old)) if (!['children', 'location', 'id'].includes(key)) mapLocations(old[key], current[key], locations);
}

async function projectReport(report, texts) {
  const scenarios = [], locations = new Map(), errors = [];
  const uris = new Set([...report.scenarios.map(s => s.uri), ...texts.keys()]);
  for (const uri of uris) {
    const recorded = report.scenarios.filter(s => s.uri === uri);
    const snapshot = report.sources?.[uri];
    let current, previous;
    try { current = await descriptors(texts.get(uri) ?? ''); }
    catch (error) { errors.push(`${uri}: ${error.message}`); }
    try { previous = Array.isArray(snapshot) ? await descriptors(snapshot.join('\n')) : []; }
    catch { previous = []; }
    const used = new Set(), matches = new Map(), fileLocations = new Map();
    for (const old of previous) {
      const exact = current?.filter(node => node.signature === old.signature) || [];
      const byName = current?.filter(node => node.name === old.name && node.rule === old.rule) || [];
      const match = exact.length === 1 ? exact[0] : byName.length === 1 ? byName[0] : undefined;
      if (match) used.add(match);
      matches.set(old.line, { match, unchanged: exact.length === 1 });
      if (exact.length === 1) mapLocations(old.nodes, match.nodes, fileLocations);
    }
    for (const scenario of recorded) {
      const entry = matches.get(scenario.line);
      // Missing snapshots cannot justify source annotations or fresh execution results.
      const sourceState = !current || !snapshot ? 'unknown' : entry?.unchanged ? 'current' : entry?.match ? 'modified' : 'removed';
      scenarios.push({ ...scenario, sourceState, displayLine: entry?.match?.line ?? scenario.line });
    }
    for (const node of current || []) {
      if (used.has(node)) continue;
      // Reports without a snapshot can still identify declarations by location/name.
      if (!snapshot && recorded.some(s => s.line === node.line && s.name === node.name)) continue;
      scenarios.push({ id: `pending:${uri}:${node.line}`, name: node.name, uri, line: node.line, displayLine: node.line,
        feature: node.feature, gherkin_rule: node.rule, sourceState: 'pending' });
    }
    locations.set(uri, fileLocations);
  }
  return { scenarios, locations, sourceErrors: errors };
}

async function readFeatures(directory, openDocuments = []) {
  const texts = new Map();
  async function walk(dir) {
    for (const entry of await fs.readdir(dir, { withFileTypes: true })) {
      const file = path.join(dir, entry.name);
      if (entry.isDirectory()) await walk(file);
      else if (entry.isFile() && file.endsWith('.feature')) texts.set(file, await fs.readFile(file, 'utf8'));
    }
  }
  await walk(directory);
  for (const doc of openDocuments) {
    const relative = path.relative(directory, doc.uri.fsPath);
    if (doc.uri.scheme === 'file' && doc.uri.fsPath.endsWith('.feature') && relative !== '..' && !relative.startsWith('..' + path.sep) && !path.isAbsolute(relative)) texts.set(doc.uri.fsPath, doc.getText());
  }
  return texts;
}
module.exports = { descriptors, projectReport, readFeatures };
