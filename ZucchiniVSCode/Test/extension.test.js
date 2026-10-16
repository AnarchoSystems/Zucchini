'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const fsSync = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const Module = require('node:module');

test('extension connects discovery, tree navigation, annotations, stale snapshots and restricted AI edits', async t => {
  const root = await fs.mkdtemp(path.join(os.tmpdir(), 'zucchini host-'));
  t.after(() => fs.rm(root, { recursive: true, force: true }));
  const features = path.join(root, 'features');
  await fs.mkdir(features);
  const feature = path.join(features, 'Shop.feature');
  const source = 'Feature: shop\n  Scenario: buy\n    Given stock\n    Then debit\n';
  await fs.writeFile(feature, source);
  await fs.writeFile(path.join(features, 'steps.yaml'), 'steps: []');
  await fs.writeFile(path.join(root, 'Shop.h'), 'struct Shop {};');
  const reportPath = path.join(root, 'rules.json');
  await fs.writeFile(reportPath, JSON.stringify({
    schema_version: '2.0.0', fixture: 'Shop', feature_directory: features, discovery_success: true, discovery_id: 'build-one',
    sources: { [feature]: source.trimEnd().split('\n') },
    scenarios: [{ id: 0, name: 'buy', test_name: 'buy', result_key: '1234567890abcdef', uri: feature, line: 2, accepted: true }],
    rules: [{ name: 'Wallet', description: 'Debit exactly', requirements: [{ tag: 'Debit' }, { tag: 'Refund' }], events: [{ kind: 'coverage', tag: 'Debit', scenario_id: 0, uri: feature, line: 4, message: 'balance checked' }] }]
  }));
  const commands = new Map(), docs = new Map(), errors = [], notifications = [], outputLines = [], commandCalls = [];
  const documentKey = file => {
    const resolved = path.resolve(file);
    if (process.platform !== 'win32') return resolved;
    try { return fsSync.realpathSync.native(resolved).toLowerCase(); }
    catch { return resolved.toLowerCase(); }
  };
  const uri = file => ({ scheme: 'file', fsPath: file, toString: () => file });
  function document(file, text) {
    const doc = { uri: uri(file), value: text, getText() { return this.value; },
      get lineCount() { return this.value.split('\n').length; },
      lineAt(line) { return { range: { line }, text: this.value.split('\n')[line] }; },
      positionAt(offset) { return { offset }; },
      async save() { await fs.writeFile(file, this.value); return true; }
    };
    docs.set(documentKey(file), doc); return doc;
  }
  const doc = document(feature, source);
  let marks, provider, hover, shown, prompt, dirtyListener, visibilityListener, decorationOptions;
  let applyEdits = true, cancelModel = false, clearCount = 0, configurationListener;
  const revealed = [];
  const tree = { async reveal(node, options) { revealed.push({ node, options }); }, visible: true, dispose() {}, onDidChangeVisibility(fn) { visibilityListener = fn; return new Disposable(); } };
  const watchers = [];
  class Disposable { dispose() {} }
  class EventEmitter extends Disposable { constructor() { super(); this.listeners = []; this.event = fn => { this.listeners.push(fn); return new Disposable(); }; } fire() { for (const fn of this.listeners) fn(); } }
  class MarkdownString { constructor() { this.value = ''; } appendText(text) { this.value += text; } appendMarkdown(text) { this.value += text; } }
  class WorkspaceEdit {
    constructor() { this.operations = []; }
    createFile(uri, options) { this.operations.push({ kind: 'create', uri, options }); }
    replace(uri, range, text) { this.operations.push({ kind: 'replace', uri, text }); }
    insert(uri, position, text) { this.operations.push({ kind: 'insert', uri, position, text }); }
  }
  const settings = { reportPatterns: ['**/rules.json'], grouping: 'rules', scenarioCount: 2 };
  const vscode = {
    version: '1.105.0',
    Uri: { file: uri }, EventEmitter, MarkdownString, WorkspaceEdit,
    Range: class { constructor(line) { this.line = line; } }, Position: class {},
    ThemeColor: class { constructor(id) { this.id = id; } }, ThemeIcon: class { constructor(id, color) { this.id = id; this.color = color; } },
    Hover: class { constructor(contents) { this.contents = contents; } },
    TreeItem: class { constructor(label, collapsibleState) { this.label = label; this.collapsibleState = collapsibleState; } },
    TreeItemCollapsibleState: { None: 0, Collapsed: 1 }, OverviewRulerLane: { Right: 4 },
    ProgressLocation: { Notification: 1 }, ConfigurationTarget: { Workspace: 2 },
    LanguageModelChatMessage: { User: text => ({ text }) },
    commands: {
      registerCommand(name, fn) { commands.set(name, fn); return new Disposable(); },
      async executeCommand(name) { commandCalls.push(name); },
    },
    workspace: {
      get textDocuments() { return [...docs.values()]; },
      isTrusted: true, workspaceFolders: [{ uri: uri(root) }],
      getWorkspaceFolder: () => ({ uri: uri(root) }),
      getConfiguration: () => ({ get: (key, fallback) => settings[key] ?? fallback, update: async (key, value) => { settings[key] = value; } }),
      findFiles: async () => [uri(reportPath)], fs: { readFile: file => fs.readFile(file.fsPath) },
      onDidChangeTextDocument(fn) { dirtyListener = fn; return new Disposable(); },
      onDidChangeConfiguration(fn) { configurationListener = fn; return new Disposable(); },
      onDidOpenTextDocument: () => new Disposable(), onDidCloseTextDocument: () => new Disposable(),
      createFileSystemWatcher(pattern) {
        const watcher = { pattern, dispose() {}, onDidCreate(fn) { this.created = fn; return new Disposable(); }, onDidChange(fn) { this.changed = fn; return new Disposable(); }, onDidDelete(fn) { this.deleted = fn; return new Disposable(); } };
        watchers.push(watcher); return watcher;
      },
      async openTextDocument(file) {
        assert.equal(file.content, undefined, 'Generation must not create untitled documents');
        return docs.get(documentKey(file.fsPath)) || document(file.fsPath, await fs.readFile(file.fsPath, 'utf8'));
      },
      async applyEdit(edit) {
        if (!applyEdits) return false;
        for (const op of edit.operations) {
          if (op.kind === 'create') document(op.uri.fsPath, '');
          else {
            const target = docs.get(documentKey(op.uri.fsPath));
            const offset = op.position?.offset ?? 0;
            target.value = op.kind === 'replace' ? op.text : target.value.slice(0, offset) + op.text + target.value.slice(offset);
          }
        }
        return true;
      }
    },
    window: {
      visibleTextEditors: [{ document: doc, setDecorations(_type, ranges) { marks = ranges; } }],
      createOutputChannel: () => ({ ...new Disposable(), dispose() {}, append() {}, appendLine(text) { outputLines.push(text); }, clear() { clearCount++; outputLines.length = 0; }, show() {} }),
      createTextEditorDecorationType: options => { decorationOptions = options; return new Disposable(); },

      onDidChangeVisibleTextEditors: () => new Disposable(),
      showErrorMessage: message => errors.push(message), showWarningMessage() {}, showInformationMessage: message => notifications.push(message),
      async showTextDocument(document, options) { shown = { document, options }; },
      showQuickPick: async items => cancelModel ? undefined : items[0],
      createTreeView(id, options) { assert.equal(id, 'zucchini.rules'); provider = options.treeDataProvider; return tree; },
      withProgress: async (_options, fn) => fn({ report() {} }, { isCancellationRequested: false })
    },
    languages: { registerHoverProvider(_selector, value) { hover = value; return new Disposable(); } },
    lm: { selectChatModels: async () => [{ name: 'Auto', id: 'auto', vendor: 'copilotcli',
      sendRequest() { assert.fail('Chat-session models must not be offered'); }
    }, { name: 'Test model', id: 'test', vendor: 'test', maxInputTokens: 100000,
      countTokens: async () => 100,
      async sendRequest(messages) {
        prompt = messages[0].text;
        return { text: (async function* () { yield JSON.stringify({ explanation: 'Check refunds', additions: [{ path: 'Shop.feature', content: '  Scenario: refund\n    Then debit' }] }); })() };
      }
    }] }
  };
  const runner = require('../src/runner');
  const originalBuild = runner.buildDirectory, originalValidate = runner.validateGeneration;
  runner.buildDirectory = async () => root;
  let validations = 0;
  runner.validateGeneration = async () => { validations++; return { ready: true, outcomes: ['passed'], diagnostics: 'refund passed' }; };
  t.after(() => { runner.buildDirectory = originalBuild; runner.validateGeneration = originalValidate; });
  const originalLoad = Module._load;
  Module._load = function(name, ...rest) { return name === 'vscode' ? vscode : originalLoad.call(this, name, ...rest); };
  t.after(() => { Module._load = originalLoad; });
  const { activate } = require('../src/extension');
  const subscriptions = [];
  activate({ subscriptions });
  t.after(() => { for (const disposable of subscriptions) disposable.dispose(); });
  await commands.get('zucchini.refresh')();
  const [fixture] = provider.getChildren();
  assert.equal(provider.getTreeItem(fixture).label, 'Shop');
  assert.equal(tree.description, 'Rules');
  await commands.get('zucchini.find')();
  assert.deepEqual(commandCalls, ['zucchini.rules.focus', 'zucchini.rules.focus', 'list.find']);
  assert.deepEqual(revealed.map(entry => entry.node.kind), ['fixture', 'rule', 'requirement']);
  assert.ok(revealed.every(entry => entry.options.expand && !entry.options.select && !entry.options.focus));
  assert.equal(provider.getParent(revealed[1].node), revealed[0].node);
  await commands.get('zucchini.showScenarios')();
  assert.equal(settings.grouping, 'scenarios');
  assert.equal(tree.description, 'Scenarios');
  assert.equal(provider.getChildren(fixture)[0].kind, 'scenario');
  await commands.get('zucchini.showRules')();
  assert.equal(settings.grouping, 'rules');
  settings.fixtureFilter = '!Shop';
  configurationListener({ affectsConfiguration: key => key === 'zucchini' });
  assert.deepEqual(provider.getChildren(), []);
  assert.match(tree.message, /0 of 1 fixtures/);
  assert.match(tree.description, /Filter: !Shop/);
  assert.equal(marks.length, 0);
  assert.equal(hover.provideHover(doc, { line: 3 }), undefined);
  await commands.get('zucchini.clearFixtureFilter')();
  assert.equal(provider.getChildren().length, 1);
  assert.equal(tree.message, undefined);
  assert.equal(marks.length, 1);
  settings.fixtureFilter = 'Other';
  configurationListener({ affectsConfiguration: key => key === 'zucchini' });
  assert.match(tree.message, /0 of 1/);
  await commands.get('zucchini.clearFixtureFilter')();
  const [rule] = provider.getChildren(fixture);
  assert.equal(provider.getTreeItem(rule).command, undefined);
  const [debit, refund] = provider.getChildren(rule);
  assert.equal(provider.getTreeItem(refund).iconPath.id, 'warning');
  assert.equal(provider.getTreeItem(refund).collapsibleState, vscode.TreeItemCollapsibleState.None, 'Empty obligations are leaves so native filtering can hide them');
  assert.equal(provider.getTreeItem(debit).collapsibleState, vscode.TreeItemCollapsibleState.Collapsed);
  const [scenario] = provider.getChildren(debit);
  assert.equal(provider.getTreeItem(scenario).command.command, 'zucchini.open');
  await commands.get('zucchini.open')(scenario);
  assert.equal(shown.document.uri.fsPath, feature);
  assert.equal(shown.options.selection.line, 1);
  assert.equal(marks.length, 1);
  assert.match(hover.provideHover(doc, { line: 3 }).contents.value, /Wallet.*Debit/);
  assert.equal(decorationOptions.backgroundColor, 'rgba(100, 180, 255, 0.16)');
  tree.visible = false; visibilityListener();
  assert.equal(marks.length, 0);
  assert.equal(hover.provideHover(doc, { line: 3 }), undefined);
  tree.visible = true; visibilityListener();
  assert.equal(marks.length, 1);
  doc.value += '\n  Scenario: Foo\n    Then debit\n'; dirtyListener();
  await commands.get('zucchini.refresh')();
  assert.equal(marks.length, 1);
  const updatedFixture = provider.getChildren()[0];
  const unassigned = provider.getChildren(updatedFixture).find(n => n.kind === 'unassigned');
  assert.equal(provider.getChildren(unassigned)[0].scenario.name, 'Foo');
  assert.match(provider.getTreeItem(provider.getChildren(unassigned)[0]).description, /Awaiting discovery/);
  outputLines.push('previous action');
  await commands.get('zucchini.run')(provider.getChildren(unassigned)[0]);
  assert.ok(!outputLines.includes('previous action'));
  assert.deepEqual(errors, []);
  doc.value = source.replace('Then debit', 'Then refund');
  dirtyListener(); await commands.get('zucchini.refresh')();
  assert.equal(marks.length, 0);
  assert.equal(hover.provideHover(doc, { line: 3 }), undefined);
  assert.equal(watchers.length, 3);
  doc.value = source;
  const resultDir = path.join(root, 'results');
  await fs.mkdir(resultDir);
  const resultPath = path.join(resultDir, '1234567890abcdef.json');
  const resultWatcher = watchers.find(w => w.pattern.includes('results'));
  async function waitFor(check) {
    for (let attempt = 0; attempt < 100; attempt++) {
      if (check()) return;
      await new Promise(resolve => setTimeout(resolve, 20));
    }
    assert.fail('Watcher did not update the view');
  }
  const currentRule = () => provider.getChildren(provider.getChildren()[0])[0];
  await fs.writeFile(resultPath, JSON.stringify({ fixture: 'Shop', test_name: 'buy', discovery_id: 'build-one', status: 'passed' }));
  const clearsBeforeWatcher = clearCount;
  resultWatcher.changed();
  await waitFor(() => provider.getTreeItem(currentRule()).iconPath.id === 'pass');
  assert.equal(clearCount, clearsBeforeWatcher, 'Automatic refresh must preserve action output');
  assert.equal(provider.getTreeItem(currentRule()).iconPath.color.id, 'testing.iconPassed');
  await fs.writeFile(resultPath, JSON.stringify({ fixture: 'Shop', test_name: 'buy', discovery_id: 'build-one', status: 'failed' }));
  resultWatcher.changed();
  await waitFor(() => provider.getTreeItem(currentRule()).iconPath.id === 'error');
  assert.equal(provider.getTreeItem(currentRule()).iconPath.color.id, 'testing.iconFailed');
  const rebuiltReport = JSON.parse(await fs.readFile(reportPath, 'utf8'));
  rebuiltReport.discovery_id = 'build-two';
  rebuiltReport.discovery_success = false;
  await fs.writeFile(reportPath, JSON.stringify(rebuiltReport));
  watchers.find(w => w.pattern === '**/rules.json').changed();
  await waitFor(() => provider.getTreeItem(currentRule()).iconPath.id === 'history');
  const rebuiltScenario = provider.getChildren(provider.getChildren(currentRule())[0])[0];
  assert.equal(provider.getTreeItem(rebuiltScenario).iconPath.id, 'history');
  assert.equal(provider.getTreeItem(provider.getChildren()[0]).iconPath.id, 'warning');
  shown = undefined;
  outputLines.push('previous generation');
  await commands.get('zucchini.examples')(rule);
  assert.ok(!outputLines.includes('previous generation'));
  assert.deepEqual(errors, []);
  assert.ok(prompt.includes('Generate 2'));
  assert.ok(docs.get(documentKey(feature)).getText().startsWith(source));
  assert.ok(docs.get(documentKey(feature)).getText().includes('Scenario: refund'));
  assert.equal(await fs.readFile(feature, 'utf8'), doc.getText());
  assert.equal(validations, 1);
  assert.deepEqual(errors, []);
  assert.equal(shown, undefined, 'Generation must not open an editor');
  assert.match(notifications.at(-1), /generation complete.*Saved and validated 1 feature file/);
  assert.ok(outputLines.some(line => line.includes('Check refunds')));
  settings.maxGenerationAttempts = 2;
  await commands.get('zucchini.counterexamples')(rule);
  assert.equal(validations, 3, 'Failing mode retries up to the configured limit');
  assert.match(prompt, /refund passed/);
  await commands.get('zucchini.exploratory')(rule);
  assert.equal(validations, 4, 'Exploration accepts an observed pass');
  doc.value = source;
  applyEdits = false;
  await commands.get('zucchini.counterexamples')(rule);
  assert.match(errors.at(-1), /Could not apply generated feature additions/);
  assert.equal(shown, undefined);

  assert.equal(provider.getTreeItem({ kind: 'scenario', fixture: rule.fixture, scenario: rule.fixture.report.scenarios[0] }).resourceUri, undefined, 'Scenario rows must not inherit file-level Git decorations');
  assert.equal(commands.has('zucchini.writeRule'), false);
});
