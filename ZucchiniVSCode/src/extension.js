'use strict';
const vscode = require('vscode');
const path = require('node:path');
const model = require('./model');
const runner = require('./runner');
const features = require('./features');
const results = require('./results');
const { requestDraft, supportsDraftRequests } = require('./generation');
const { collectContext, generationPrompt, insideWorkspace, planAdditions } = require('./context');

function activate(context) {
  const output = vscode.window.createOutputChannel('Zucchini');
  const changes = new vscode.EventEmitter();
  const decoration = vscode.window.createTextEditorDecorationType({
    isWholeLine: true, backgroundColor: 'rgba(100, 180, 255, 0.16)',
    overviewRulerColor: new vscode.ThemeColor('editorInfo.foreground'), overviewRulerLane: vscode.OverviewRulerLane.Right
  });
  let fixtures = [];
  let refreshSequence = 0;
  let running = false;
  const statuses = new Map();
  const config = () => vscode.workspace.getConfiguration('zucchini');
  const roots = () => (vscode.workspace.workspaceFolders || []).filter(f => f.uri.scheme === 'file').map(f => f.uri.fsPath);
  const options = fixture => {
    const settings = vscode.workspace.getConfiguration('zucchini', vscode.Uri.file(fixture.path)).get('fixtures', {});
    return settings[fixture.path] || settings[fixture.report.fixture] || {};
  };
  const statusKey = (fixture, scenario) => `${fixture.path}:${scenario.id}`;
  const safe = fn => async (...args) => { try { return await fn(...args); } catch (error) { output.appendLine(error.stack || String(error)); vscode.window.showErrorMessage(`Zucchini: ${error.message}`); } };

  const visibleFixtures = () => model.filterFixtures(fixtures, config().get('fixtureFilter', ''));
  function updateView() {
    const grouping = config().get('grouping', 'rules');
    const filter = config().get('fixtureFilter', '');
    tree.description = `${grouping === 'scenarios' ? 'Scenarios' : 'Rules'}${filter ? ` · Filter: ${filter}` : ''}`;
    tree.message = filter ? `${visibleFixtures().length} of ${fixtures.length} fixtures · ${filter}` : undefined;
    changes.fire(); highlight();
  }

  let tree;
  let refreshTimer;
  function scheduleRefresh() {
    clearTimeout(refreshTimer);
    refreshTimer = setTimeout(() => safe(refresh)(), 150);
  }
  function status(fixture, scenario) {
    return results.scenarioStatus(fixture, scenario, statuses.get(statusKey(fixture, scenario)));
  }
  function observations(fixture, document, line) {
    const mapping = fixture.locations?.get(document.uri.fsPath);
    if (!mapping || fixture.sourceTexts?.get(document.uri.fsPath) !== document.getText()) return [];
    return model.businessRules(fixture.report).flatMap(rule => rule.events.filter(event =>
      event.uri === document.uri.fsPath && mapping.get(event.line) === line &&
      model.allScenarios(fixture).some(s => s.id === event.scenario_id && s.sourceState === 'current')
    ).map(event => ({ rule, event })));
  }
  function highlight() {
    for (const editor of vscode.window.visibleTextEditors) {
      const lines = new Set();
      if (tree?.visible) for (const fixture of visibleFixtures()) {
        const mapping = fixture.locations?.get(editor.document.uri.fsPath);
        if (!mapping) continue;
        for (const rule of model.businessRules(fixture.report)) for (const event of rule.events) {
          const line = event.uri === editor.document.uri.fsPath ? mapping.get(event.line) : undefined;
          if (line && line <= editor.document.lineCount && observations(fixture, editor.document, line).length) lines.add(line - 1);
        }
      }
      editor.setDecorations(decoration, [...lines].map(line => editor.document.lineAt(line).range));
    }
  }

  const provider = {
    onDidChangeTreeData: changes.event,
    getChildren(node) {
      return node ? model.children(node, config().get('grouping', 'rules')).map(child => ({ ...child, parent: node }))
        : visibleFixtures().map(fixture => ({ kind: 'fixture', fixture }));
    },
    getParent(node) { return node.parent; },
    getTreeItem(node) {
      const { fixture, rule, requirement, scenario } = node;
      let label, description, icon, tooltip, execution;
      const expandable = model.children(node, config().get('grouping', 'rules')).length > 0;
      switch (node.kind) {
        case 'fixture':
          label = fixture.report.fixture;
          description = `${model.allScenarios(fixture).length} scenarios · ${path.relative(vscode.workspace.getWorkspaceFolder(vscode.Uri.file(fixture.path))?.uri.fsPath || '', fixture.path)}`;
          icon = fixture.report.discovery_success === false ? 'warning' : 'package';
          tooltip = fixture.path + (fixture.sourceErrors?.length ? '\n' + fixture.sourceErrors.join('\n') : '') + (fixture.report.discovery_success === false ? '\nDiscovery failed. Report may be incomplete.\n' + JSON.stringify(fixture.report.discovery_diagnostics) : '');
          break;
        case 'rule': case 'requirement': {
          label = requirement ? requirement.tag : rule.name;
          const witnesses = model.witnesses(fixture.report, rule, requirement).length;
          const gaps = requirement ? (witnesses ? 0 : 1) : rule.requirements.filter(r => !model.witnesses(fixture.report, rule, r).length).length;
          description = `${witnesses} witnesses${gaps ? ` · ${gaps} unwitnessed obligation${gaps === 1 ? '' : 's'}` : ''}`;
          const executions = model.scenariosUnder(node).map(s => status(fixture, s));
          execution = results.aggregateStatus(executions);
          icon = execution === 'failed' ? 'error' : execution === 'passed' ? 'pass' : execution === 'running' ? 'loading~spin' : execution === 'stale' ? 'history' : gaps || !witnesses ? 'warning' : 'checklist';
          if (execution !== 'not_run') description += ` · ${execution} (${executions.filter(s => ['passed', 'failed', 'skipped'].includes(s)).length}/${executions.length} run)`;
          tooltip = `${requirement?.description || rule.description || label}\n${rule.kind || 'business'} rule\n${description}\nWitnesses are discovery coverage observations, not proof of runtime success.`;
          break;
        }
        case 'unassigned': label = 'Scenarios without business rule observations'; icon = 'list-unordered'; break;
        case 'scenario': {
          label = scenario.name;
          const participation = model.participation(fixture.report, scenario);
          description = participation.map(p => `${p.rule.name}${p.tags.length ? ` (${p.tags.join(', ')})` : ''}`).join('; ') || 'No rule observations';
          execution = status(fixture, scenario);
          if (scenario.sourceState === 'pending') description = 'Awaiting discovery · rebuild manually';
          else if (scenario.sourceState && scenario.sourceState !== 'current') description += ` · ${scenario.sourceState} since discovery`;
          icon = execution === 'passed' ? 'pass' : execution === 'failed' ? 'error' : execution === 'running' ? 'loading~spin' : execution === 'stale' ? 'history' : execution === 'rejected' ? 'circle-slash' : execution === 'pending' ? 'clock' : 'beaker';
          tooltip = `${scenario.uri}:${scenario.line}\n${description}\n${scenario.sourceState === 'pending' ? 'Not discovered yet' : scenario.accepted === false ? 'Rejected during discovery' : 'Accepted during discovery'}\nExecution: ${execution}`;
          break;
        }
      }
      const item = new vscode.TreeItem(label, expandable ? vscode.TreeItemCollapsibleState.Collapsed : vscode.TreeItemCollapsibleState.None);
      item.description = description;
      item.tooltip = tooltip;
      item.contextValue = node.kind;
      item.iconPath = new vscode.ThemeIcon(icon, execution === 'passed' ? new vscode.ThemeColor('testing.iconPassed') : execution === 'failed' ? new vscode.ThemeColor('testing.iconFailed') : icon === 'warning' ? new vscode.ThemeColor('list.warningForeground') : undefined);
      if (scenario) {
        item.resourceUri = vscode.Uri.file(scenario.uri);
        item.command = { command: 'zucchini.open', title: 'Open Scenario', arguments: [node] };
      }
      return item;
    }
  };

  async function refresh() {
    const sequence = ++refreshSequence;
    const paths = new Map();
    for (const pattern of config().get('reportPatterns', [])) {
      for (const uri of await vscode.workspace.findFiles(pattern, '**/{.git,node_modules}/**')) paths.set(uri.toString(), uri);
    }
    const found = [];
    const errors = [];
    for (const uri of [...paths.values()].sort((a, b) => a.fsPath.localeCompare(b.fsPath))) {
      try {
        await insideWorkspace(uri.fsPath, roots());
        const text = Buffer.from(await vscode.workspace.fs.readFile(uri)).toString('utf8');
        const fixture = { path: uri.fsPath, report: model.parseReport(text) };
        fixture.results = await results.loadResults(fixture);
        try {
          await insideWorkspace(fixture.report.feature_directory, roots());
          const texts = await features.readFeatures(fixture.report.feature_directory, vscode.workspace.textDocuments || []);
          Object.assign(fixture, await features.projectReport(fixture.report, texts));
          fixture.sourceTexts = texts;
        } catch (error) { fixture.sourceErrors = [error.message]; }
        found.push(fixture);
      } catch (error) { errors.push(`${uri.fsPath}: ${error.message}`); }
    }
    if (sequence !== refreshSequence) return;
    fixtures = found;
    updateView();
    for (const error of errors) output.appendLine(error);
    if (errors.length) vscode.window.showWarningMessage(`Zucchini: ${errors.length} report(s) could not be loaded. See the Zucchini output channel.`);
  }

  async function run(node) {
    if (!vscode.workspace.isTrusted) throw new Error('Trust the workspace before executing scenarios.');
    if (!node) throw new Error('Select a fixture, rule, obligation or scenario first.');
    if (running) throw new Error('A scenario run is already in progress.');
    // Refresh first: a rebuild may have replaced discovery data since this node was rendered.
    await refresh();
    const current = fixtures.find(f => f.path === node.fixture.path);
    if (!current) throw new Error('The fixture report is no longer available.');
    if (node.fixture.report.discovery_id !== current.report.discovery_id) throw new Error('Discovery changed. Select the refreshed scenario or hierarchy and run again.');
    node = { ...node, fixture: current };
    if (node.rule) {
      node.rule = model.businessRules(current.report).find(rule => rule.name === node.rule.name);
      if (!node.rule) throw new Error('This rule is no longer in the discovery report.');
      if (node.requirement) node.requirement = node.rule.requirements.find(r => r.tag === node.requirement.tag);
      if (node.kind === 'requirement' && !node.requirement) throw new Error('This obligation is no longer in the discovery report.');
    }
    if (node.scenario) node.scenario = model.allScenarios(current).find(s => s.id === node.scenario.id);
    if (node.kind === 'scenario' && !node.scenario) throw new Error('Scenario changed during discovery. Select it again.');
    const candidates = model.scenariosUnder(node);
    const scenarios = candidates.filter(s => s.sourceState !== 'pending' && (!s.sourceState || s.sourceState === 'current') && s.accepted !== false);
    if (scenarios.length !== candidates.length) vscode.window.showInformationMessage('Zucchini: scenarios awaiting discovery, edited scenarios, and discovery rejections are skipped. Rebuild manually to update them.');
    if (!scenarios.length) { vscode.window.showInformationMessage('No witnessing scenarios to run.'); return; }
    running = true;
    try {
      await vscode.window.withProgress({ location: vscode.ProgressLocation.Notification, title: 'Zucchini scenarios', cancellable: true }, async (progress, token) => {
        const fixture = node.fixture;
        const settings = options(fixture);
        const directory = await runner.buildDirectory(fixture.path, settings.buildDirectory);
        const configuration = settings.configuration || (/^(Debug|Release|RelWithDebInfo|MinSizeRel)$/i.test(path.basename(path.dirname(fixture.path))) ? path.basename(path.dirname(fixture.path)) : '');
        const base = ['--test-dir', directory, ...(configuration ? ['-C', configuration] : [])];
        const ctest = config().get('ctestPath', 'ctest');
        const discovery = await runner.processOutput(ctest, [...base, '--show-only=json-v1'], undefined, token);
        if (token.isCancellationRequested) return;
        if (discovery.code !== 0) throw new Error(`CTest discovery failed: ${discovery.output}`);
        const latest = model.parseReport(Buffer.from(await vscode.workspace.fs.readFile(vscode.Uri.file(fixture.path))).toString('utf8'));
        if (latest.discovery_id !== fixture.report.discovery_id) throw new Error('CTest refreshed discovery. Select the updated scenarios and run again.');
        const tests = JSON.parse(discovery.output).tests;
        // Resolve the complete selection before starting any test.
        const selected = scenarios.map(scenario => ({ scenario, test: runner.selectTest(tests, fixture.report.fixture, scenario) }));
        output.show(true);
        for (const { scenario, test } of selected) {
          if (token.isCancellationRequested) break;
          progress.report({ message: scenario.name, increment: 100 / selected.length });
          statuses.set(statusKey(fixture, scenario), { discovery_id: fixture.report.discovery_id, status: 'running' }); changes.fire();
          output.appendLine(`\nRunning ${test.name}`);
          try {
            const result = await runner.processOutput(ctest, [...base, '-R', runner.exactRegex(test.name), '--no-tests=error', '--output-on-failure'], text => output.append(text), token);
            statuses.set(statusKey(fixture, scenario), { discovery_id: fixture.report.discovery_id, status: token.isCancellationRequested ? 'cancelled' : result.code === 0 ? 'passed' : 'failed' });
          } catch (error) { statuses.set(statusKey(fixture, scenario), { discovery_id: fixture.report.discovery_id, status: 'incomplete' }); throw error; }
          finally { changes.fire(); }
        }
      });
    } finally { running = false; await refresh(); }
  }

  async function pickModel(placeHolder) {
    const available = await vscode.lm.selectChatModels({});
    const models = available.filter(supportsDraftRequests);
    if (!models.length) throw new Error(available.length
      ? 'Only Copilot CLI chat-session models are available. They cannot answer extension requests. Enable GitHub Copilot Chat models (provider: copilot) or another language model provider in VS Code.'
      : 'No VS Code language models are available. Configure a language model provider in VS Code first.');
    return vscode.window.showQuickPick(models.map(m => ({ label: m.name, description: `${m.vendor} · ${m.id}`, model: m })), { placeHolder });
  }

  async function generate(node, adversarial) {
    if (!vscode.workspace.isTrusted) throw new Error('Trust the workspace before sending fixture context to AI.');
    if (!node?.rule) throw new Error('Select a rule or obligation.');
    const picked = await pickModel('Choose a model for scenario generation');
    if (!picked) return;
    await vscode.window.withProgress({ location: vscode.ProgressLocation.Notification, title: 'Generating Zucchini scenario draft', cancellable: true }, async (progress, token) => {
      const sourceContext = await collectContext(node.fixture, options(node.fixture), roots(), async file => (await vscode.workspace.openTextDocument(vscode.Uri.file(file))).getText());
      const prompt = generationPrompt(node, config().get('scenarioCount', 3), adversarial, sourceContext);
      const draft = await requestDraft(picked.model, prompt, token, {
        message: text => vscode.LanguageModelChatMessage.User(text),
        log: text => output.appendLine(text),
        progress: message => progress.report({ message })
      });
      if (draft === undefined) return;
      const result = await planAdditions(draft, node.fixture.report.feature_directory, roots(), async file => (await vscode.workspace.openTextDocument(vscode.Uri.file(file))).getText());
      if (token.isCancellationRequested) return;
      const edit = new vscode.WorkspaceEdit();
      for (const change of result.changes) {
        const snapshot = sourceContext.files.find(file => file.file === change.file);
        if (change.existing && (!snapshot || snapshot.text !== change.before)) throw new Error('A feature changed during generation. Retry with current context.');
      }
      for (const change of result.changes) {
        const uri = vscode.Uri.file(change.file);
        if (change.existing) {
          const doc = await vscode.workspace.openTextDocument(uri);
          if (doc.getText() !== change.before) throw new Error('A feature changed during generation. Retry with current context.');
          edit.insert(uri, doc.positionAt(change.before.length), change.after.slice(change.before.length));
        } else {
          edit.createFile(uri, { overwrite: false });
          edit.insert(uri, new vscode.Position(0, 0), change.after);
        }
      }
      if (result.changes.length && !await vscode.workspace.applyEdit(edit)) throw new Error('Could not apply generated feature additions.');
      output.appendLine(`${adversarial ? 'Counterexamples' : 'Examples'}: ${node.requirement?.tag || node.rule.name}\n${result.explanation}`);
      for (const change of result.changes) output.appendLine(`Added scenarios: ${change.file}`);
      vscode.window.showInformationMessage(result.changes.length
        ? `Zucchini: scenario generation complete. Updated ${result.changes.length} feature file(s) in editor buffers. Review, save and rebuild manually.`
        : 'Zucchini: scenario generation complete. No feature files were changed. See the Zucchini output channel for details.');
    });
  }

  tree = vscode.window.createTreeView('zucchini.rules', { treeDataProvider: provider, showCollapseAll: true });
  context.subscriptions.push(output, changes, decoration, tree,
    tree.onDidChangeVisibility(highlight),
    { dispose() { clearTimeout(refreshTimer); } },
    vscode.window.onDidChangeVisibleTextEditors(highlight),
    vscode.workspace.onDidChangeTextDocument(scheduleRefresh),
    vscode.workspace.onDidOpenTextDocument(scheduleRefresh),
    vscode.workspace.onDidCloseTextDocument(scheduleRefresh),
    vscode.workspace.onDidChangeConfiguration(event => { if (event.affectsConfiguration('zucchini.reportPatterns')) safe(refresh)(); else if (event.affectsConfiguration('zucchini')) updateView(); }),
    vscode.languages.registerHoverProvider({ scheme: 'file' }, {
      provideHover(document, position) {
        if (!tree.visible) return;
        const entries = visibleFixtures().flatMap(fixture => observations(fixture, document, position.line + 1).map(entry => ({ ...entry, fixture })));
        if (!entries.length) return;
        const markdown = new vscode.MarkdownString();
        const seen = new Set();
        for (const { fixture, rule, event } of entries) {
          const key = `${fixture.path}:${rule.name}:${event.tag}:${event.message}`;
          if (seen.has(key)) continue; seen.add(key);
          markdown.appendText(`${fixture.report.fixture} · ${rule.name}${event.tag ? ` / ${event.tag}` : ''}\n${event.kind}: ${event.message || ''}`);
          markdown.appendMarkdown('\n\n');
        }
        return new vscode.Hover(markdown, document.lineAt(position.line).range);
      }
    })
  );
  function command(name, callback, clearOutput = false) {
    context.subscriptions.push(vscode.commands.registerCommand(`zucchini.${name}`, safe(async (...args) => {
      if (clearOutput) output.clear();
      return callback(...args);
    })));
  }
  command('refresh', refresh, true);
  command('group', async () => {
    const value = await vscode.window.showQuickPick([{ label: 'Rules → obligations → scenarios', value: 'rules' }, { label: 'Flat scenarios with rule participation', value: 'scenarios' }]);
    if (value) await setGrouping(value.value);
  });
  async function setGrouping(value) {
    await config().update('grouping', value, vscode.ConfigurationTarget.Workspace);
    updateView();
  }
  command('showScenarios', () => setGrouping('scenarios'));
  command('showRules', () => setGrouping('rules'));
  command('find', async () => {
    await vscode.commands.executeCommand('zucchini.rules.focus');
    // Native find cannot prune branches whose children have never been loaded.
    // Materialize each branch before searching the nested rules hierarchy.
    async function expand(node) {
      const children = provider.getChildren(node);
      if (!children.length) return;
      await tree.reveal(node, { expand: true, select: false, focus: false });
      for (const child of children) await expand(child);
    }
    for (const node of provider.getChildren()) await expand(node);
    await vscode.commands.executeCommand('zucchini.rules.focus');
    await vscode.commands.executeCommand('list.find');
  });
  command('clearFixtureFilter', async () => {
    await config().update('fixtureFilter', '', vscode.ConfigurationTarget.Workspace);
    updateView();
  });
  command('open', async node => {
    if (!node?.scenario) return;
    const document = await vscode.workspace.openTextDocument(vscode.Uri.file(node.scenario.uri));
    const line = Math.min((node.scenario.displayLine || node.scenario.line) - 1, document.lineCount - 1);
    await vscode.window.showTextDocument(document, { selection: new vscode.Range(line, 0, line, 0) });
    if (node.scenario.sourceState && node.scenario.sourceState !== 'current') vscode.window.showWarningMessage('Zucchini: this feature differs from the discovery report. Rebuild manually and refresh to update locations and rule observations.');
  });
  command('run', run, true);
  command('generate', async node => {
    const mode = await vscode.window.showQuickPick([{ label: 'Generate passing examples', adversarial: false }, { label: 'Play devil’s advocate: generate failing counterexamples', adversarial: true }]);
    if (mode) await generate(node, mode.adversarial);
  }, true);
  command('examples', node => generate(node, false), true);
  command('counterexamples', node => generate(node, true), true);
  for (const pattern of ['**/rules.json', '**/results/*.json', '**/*.feature']) {
    const watcher = vscode.workspace.createFileSystemWatcher(pattern);
    context.subscriptions.push(watcher, watcher.onDidCreate(scheduleRefresh), watcher.onDidChange(scheduleRefresh), watcher.onDidDelete(scheduleRefresh));
  }
  updateView();
  safe(refresh)();
}
module.exports = { activate };
