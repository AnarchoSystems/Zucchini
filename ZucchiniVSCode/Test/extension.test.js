'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
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
  const uri = file => ({ scheme: 'file', fsPath: file, toString: () => file });
  function document(file, text) {
    const doc = { uri: uri(file), value: text, getText() { return this.value; },
      get lineCount() { return this.value.split('\n').length; },
      lineAt(line) { return { range: { line }, text: this.value.split('\n')[line] }; },
      positionAt(offset) { return { offset }; }
    };
    docs.set(file, doc); return doc;
  }
  const doc = document(feature, source);
  let marks, provider, hover, shown, prompt, dirtyListener, visibilityListener, decorationOptions;
  let applyEdits = true, cancelModel = false, clearCount = 0, configurationListener, filterProvider, filterMessage;
  const tree = { visible: true, dispose() {}, onDidChangeVisibility(fn) { visibilityListener = fn; return new Disposable(); } };
  const watchers = [];
  class Disposable { dispose() {} }
  class EventEmitter extends Disposable { constructor() { super(); this.event = () => new Disposable(); } fire() {} }
  class MarkdownString { constructor() { this.value = ''; } appendText(text) { this.value += text; } appendMarkdown(text) { this.value += text; } }
  class WorkspaceEdit {
    constructor() { this.operations = []; }
    createFile(uri, options) { this.operations.push({ kind: 'create', uri, options }); }
    insert(uri, position, text) { this.operations.push({ kind: 'insert', uri, position, text }); }
  }
  const settings = { reportPatterns: ['**/rules.json'], grouping: 'rules', scenarioCount: 2 };
  const vscode = {
    version: '1.105.0',
    Uri: { file: uri }, EventEmitter, MarkdownString, WorkspaceEdit,
    Range: class { constructor(line) { this.line = line; } }, Position: class {},
    ThemeColor: class { constructor(id) { this.id = id; } }, ThemeIcon: class { constructor(id, color) { this.id = id; this.color = color; } },
    Hover: class { constructor(contents) { this.contents = contents; } },
    TreeItem: class { constructor(label) { this.label = label; } },
    TreeItemCollapsibleState: { None: 0, Collapsed: 1 }, OverviewRulerLane: { Right: 4 },
    ProgressLocation: { Notification: 1 }, ConfigurationTarget: { Workspace: 2 },
    LanguageModelChatMessage: { User: text => ({ text }) },
    commands: {
      registerCommand(name, fn) { commands.set(name, fn); return new Disposable(); },

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
        return docs.get(file.fsPath) || document(file.fsPath, await fs.readFile(file.fsPath, 'utf8'));
      },
      async applyEdit(edit) {
        if (!applyEdits) return false;
        for (const op of edit.operations) {
          if (op.kind === 'create') document(op.uri.fsPath, '');
          else {
            const target = docs.get(op.uri.fsPath);
            const offset = op.position.offset ?? 0;
            target.value = target.value.slice(0, offset) + op.text + target.value.slice(offset);
          }
        }
        return true;
      }
    },
    window: {
      visibleTextEditors: [{ document: doc, setDecorations(_type, ranges) { marks = ranges; } }],
      createOutputChannel: () => ({ ...new Disposable(), dispose() {}, append() {}, appendLine(text) { outputLines.push(text); }, clear() { clearCount++; outputLines.length = 0; }, show() {} }),
      createTextEditorDecorationType: options => { decorationOptions = options; return new Disposable(); },
      createTreeView(_name, options) { provider = options.treeDataProvider; return tree; },
      onDidChangeVisibleTextEditors: () => new Disposable(),
      showErrorMessage: message => errors.push(message), showWarningMessage() {}, showInformationMessage: message => notifications.push(message),
      async showTextDocument(document, options) { shown = { document, options }; },
      showQuickPick: async items => cancelModel ? undefined : items[0],
      registerWebviewViewProvider(id, value) { assert.equal(id, 'zucchini.filter'); filterProvider = value; return new Disposable(); },
      withProgress: async (_options, fn) => fn({}, { isCancellationRequested: false })
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
  await commands.get('zucchini.showScenarios')();
  assert.equal(settings.grouping, 'scenarios');
  assert.equal(tree.description, 'Scenarios');
  assert.equal(provider.getChildren(fixture)[0].kind, 'scenario');
  await commands.get('zucchini.showRules')();
  assert.equal(settings.grouping, 'rules');
  filterProvider.resolveWebviewView({ webview: { onDidReceiveMessage(fn) { filterMessage = fn; return new Disposable(); }, postMessage() {} } });
  await filterMessage({ type: 'filter', value: '!Shop' });
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
  assert.ok(prompt.includes('Generate 2'));
  assert.ok(docs.get(feature).getText().startsWith(source));
  assert.ok(docs.get(feature).getText().includes('Scenario: refund'));
  assert.equal(await fs.readFile(feature, 'utf8'), source); // changes are unsaved editor buffers
  assert.deepEqual(errors, []);
  assert.equal(shown, undefined, 'Generation must not open an editor');
  assert.match(notifications.at(-1), /generation complete.*Updated 1 feature file/);
  assert.ok(outputLines.some(line => line.includes('Check refunds')));
  doc.value = source;
  applyEdits = false;
  await commands.get('zucchini.counterexamples')(rule);
  assert.match(errors.at(-1), /Could not apply generated feature additions/);
  assert.equal(shown, undefined);

  assert.equal(commands.has('zucchini.writeRule'), false);
});
