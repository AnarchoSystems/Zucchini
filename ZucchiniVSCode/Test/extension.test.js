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
  const source = 'Feature: shop\n  Scenario: buy\n    Given stock\n    Then debit\n    And receipt\n      | item | note | shelf |\n      | tea  |      | A1    |\n';
  await fs.writeFile(feature, source);
  await fs.writeFile(path.join(features, 'steps.yaml'), 'steps: []');
  await fs.writeFile(path.join(root, 'Shop.h'), 'struct Shop {};');
  const reportPath = path.join(root, 'rules.json');
  const reportPaths = [reportPath];
  await fs.writeFile(reportPath, JSON.stringify({
    schema_version: '2.0.0', fixture: 'Shop', executable: 'ShopTests.exe', feature_directory: features, discovery_success: true, discovery_id: 'build-one',
    sources: { [feature]: source.trimEnd().split('\n') },
    scenarios: [{ id: 0, name: 'buy', test_name: 'buy', result_key: '1234567890abcdef', uri: feature, line: 2, accepted: true,
      steps: [{ text: 'stock', line: 3 }, { text: 'debit', line: 4 }, { text: 'receipt', line: 5, argument: { kind: 'dataTable',
        rows: [{ item: 'tea', note: '', shelf: 'A1' }], ignoredKeys: ['shelf'],
        cellLocations: [{ item: { line: 7, column: 9 }, note: { line: 7, column: 21 }, shelf: { line: 7, column: 23 } }] } }],
      asserted_facts: 3, unjustified_facts: [{ step_index: 2, line: 7, column: 21, cell: { row: 0, key: 'note' } }] }],
    assertion_justification: { enforced: false, scenarios: 1, asserted_facts: 2, unjustified_facts: 1 },
    rules: [{ name: 'Wallet', description: 'Debit exactly', requirements: [{ tag: 'Debit', strictness: 'exact' }, { tag: 'Refund' }], events: [
      { kind: 'coverage', tag: 'Debit', scenario_id: 0, uri: feature, line: 4, message: 'balance checked' },
      { kind: 'coverage', tag: 'Debit', scenario_id: 0, uri: feature, line: 5, message: 'item listed', cell: { row: 0, key: 'item', line: 7, column: 9 } }] }]
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
  let marks, assertionMarks, cellMarks = {}, provider, statisticsProvider, hover, pick, shown, prompt, dirtyListener, visibilityListener, decorationOptions;
  let applyEdits = true, cancelModel = false, clearCount = 0, configurationListener;
  const refundReply = JSON.stringify({ explanation: 'Check refunds', additions: [{ path: 'Shop.feature', content: '  Scenario: refund\n    Then debit' }] });
  let reply = () => refundReply, outcome = 'passed';
  const prompts = [], toolResults = [], warnings = [];
  class LanguageModelTextPart { constructor(value) { this.value = value; } }
  class LanguageModelToolCallPart { constructor(callId, name, input) { this.callId = callId; this.name = name; this.input = input; } }
  class LanguageModelToolResultPart { constructor(callId, content) { this.callId = callId; this.content = content; } }
  const revealed = [], opened = [];
  const tree = { async reveal(node, options) { revealed.push({ node, options }); }, visible: true, dispose() {}, onDidChangeVisibility(fn) { visibilityListener = fn; return new Disposable(); } };
  const watchers = [], decorationTypes = [];
  class Disposable { dispose() {} }
  class EventEmitter extends Disposable { constructor() { super(); this.listeners = []; this.event = fn => { this.listeners.push(fn); return new Disposable(); }; } fire() { for (const fn of this.listeners) fn(); } }
  class MarkdownString { constructor() { this.value = ''; } appendText(text) { this.value += text; } appendMarkdown(text) { this.value += text; } }
  class WorkspaceEdit {
    constructor() { this.operations = []; }
    createFile(uri, options) { this.operations.push({ kind: 'create', uri, options }); }
    replace(uri, range, text) { this.operations.push({ kind: 'replace', uri, text }); }
    insert(uri, position, text) { this.operations.push({ kind: 'insert', uri, position, text }); }
    deleteFile(uri) { this.operations.push({ kind: 'delete', uri }); }
  }
  const settings = { reportPatterns: ['**/rules.json'], grouping: 'rules', scenarioCount: 2 };
  const vscode = {
    version: '1.105.0',
    Uri: { file: uri, parse: value => ({ scheme: 'https', value, toString: () => value }) }, EventEmitter, MarkdownString, WorkspaceEdit,
    Range: class { constructor(line, start, _endLine, end) { this.line = line; this.start = start; this.end = end; } }, Position: class {},
    ThemeColor: class { constructor(id) { this.id = id; } }, ThemeIcon: class { constructor(id, color) { this.id = id; this.color = color; } },
    Hover: class { constructor(contents) { this.contents = contents; } },
    TreeItem: class { constructor(label, collapsibleState) { this.label = label; this.collapsibleState = collapsibleState; } },
    TreeItemCollapsibleState: { None: 0, Collapsed: 1 }, OverviewRulerLane: { Right: 4 },
    ProgressLocation: { Notification: 1 }, ConfigurationTarget: { Workspace: 2 },
    LanguageModelTextPart, LanguageModelToolCallPart, LanguageModelToolResultPart,
    LanguageModelChatMessage: {
      User: content => typeof content === 'string' ? { text: content } : { role: 'user', content },
      Assistant: content => ({ role: 'assistant', content })
    },
    commands: {
      registerCommand(name, fn) { commands.set(name, fn); return new Disposable(); },
      async executeCommand(name) { commandCalls.push(name); },
    },
    workspace: {
      get textDocuments() { return [...docs.values()]; },
      isTrusted: true, workspaceFolders: [{ uri: uri(root) }],
      getWorkspaceFolder: () => ({ uri: uri(root) }),
      getConfiguration: () => ({ get: (key, fallback) => settings[key] ?? fallback, update: async (key, value) => { settings[key] = value; } }),
      findFiles: async () => reportPaths.map(uri), fs: { readFile: file => fs.readFile(file.fsPath) },
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
          else if (op.kind === 'delete') { docs.delete(documentKey(op.uri.fsPath)); await fs.rm(op.uri.fsPath); }
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
      visibleTextEditors: [{ document: doc, setDecorations(type, ranges) {
        const index = decorationTypes.indexOf(type);
        if (index === 0) marks = ranges; else if (index === 1) assertionMarks = ranges;
        else cellMarks[['covered', 'ignored', 'unjustified'][index - 2]] = ranges;
      } }],
      createOutputChannel: () => ({ ...new Disposable(), dispose() {}, append() {}, appendLine(text) { outputLines.push(text); }, clear() { clearCount++; outputLines.length = 0; }, show() {} }),
      createTextEditorDecorationType: options => {
        const type = new Disposable();
        decorationTypes.push(type);
        decorationOptions ||= options;
        return type;
      },

      onDidChangeVisibleTextEditors: () => new Disposable(),
      showErrorMessage: message => errors.push(message), showWarningMessage: message => warnings.push(message), showInformationMessage: message => notifications.push(message),
      async showTextDocument(document, options) { shown = { document, options }; },
      showQuickPick: async items => cancelModel ? undefined : pick ? items.find(pick) : items[0],
      createTreeView(id, options) { assert.equal(id, 'zucchini.rules'); provider = options.treeDataProvider; return tree; },
      registerWebviewViewProvider(id, value) { assert.equal(id, 'zucchini.statistics'); statisticsProvider = value; return new Disposable(); },
      withProgress: async (_options, fn) => fn({ report() {} }, { isCancellationRequested: false })
    },
    env: { openExternal: async target => { opened.push(target.toString()); return true; } },
    languages: { registerHoverProvider(_selector, value) { hover = value; return new Disposable(); } },
    lm: { selectChatModels: async () => [{ name: 'Auto', id: 'auto', vendor: 'copilotcli',
      sendRequest() { assert.fail('Chat-session models must not be offered'); }
    }, { name: 'Test model', id: 'test', vendor: 'test', maxInputTokens: 100000,
      countTokens: async () => 100,
      async sendRequest(messages, options) {
        prompt = messages[0].text;
        // With source tools, read the fixture header first, then answer.
        const explore = options.tools && !messages.some(message => message.role === 'assistant');
        if (!explore) prompts.push(prompt);
        for (const message of messages) for (const part of message.content || []) {
          if (part instanceof LanguageModelToolResultPart) toolResults.push(part.content.map(text => text.value).join(''));
        }
        const parts = explore ? [new LanguageModelToolCallPart('read-1', 'zucchini_read_file', { path: 'Shop.h' })] : [new LanguageModelTextPart(reply(prompt))];
        return { stream: (async function* () { yield* parts; })(), text: (async function* () { for (const part of parts) if (part.value) yield part.value; })() };
      }
    }] }
  };
  const runner = require('../src/runner');
  const originalBuild = runner.buildDirectory, originalValidate = runner.validateGeneration;
  runner.buildDirectory = async () => root;
  let validations = 0;
  // New scenarios get the current outcome and coverage of the obligations `covers` names.
  let covers = () => ['Refund'];
  runner.validateGeneration = async options => {
    assert.equal(options.target, settings.fixtures?.Shop?.buildTarget || 'ShopTests');
    validations++;
    const results = [], events = [];
    for (const change of options.changes) {
      const appended = change.before ? change.before.split('\n').length : 0;
      change.after.split('\n').forEach((text, index) => {
        const name = /Scenario: (.*)/.exec(text)?.[1];
        if (!name || index < appended) return;
        const id = 100 + results.length;
        results.push({ scenario: { id, name, uri: change.file, line: index + 1 }, outcome });
        for (const tag of covers(name)) events.push({ kind: 'coverage', tag, scenario_id: id });
      });
    }
    return { ready: true, outcomes: [outcome], results, diagnostics: `refund ${outcome}`,
      report: { rules: [{ name: 'Wallet', requirements: [{ tag: 'Debit' }, { tag: 'Refund' }], events }] } };
  };
  t.after(() => { runner.buildDirectory = originalBuild; runner.validateGeneration = originalValidate; });
  const originalLoad = Module._load;
  Module._load = function(name, ...rest) { return name === 'vscode' ? vscode : originalLoad.call(this, name, ...rest); };
  t.after(() => { Module._load = originalLoad; });
  const { activate } = require('../src/extension');
  const subscriptions = [], workspaceState = new Map();
  activate({ subscriptions, workspaceState: { get: key => workspaceState.get(key), update: async (key, value) => { workspaceState.set(key, value); } } });
  t.after(() => { for (const disposable of subscriptions) disposable.dispose(); });
  // The selected fixture heads the tree; its rules follow without a fixture level.
  const roots = () => provider.getChildren().filter(node => node.kind !== 'selection');
  await commands.get('zucchini.refresh')();
  const [selection] = provider.getChildren();
  assert.equal(selection.kind, 'selection');
  assert.equal(provider.getTreeItem(selection).label, 'Shop');
  assert.match(provider.getTreeItem(selection).description, /^Selected fixture · 1 scenarios/);
  assert.equal(provider.getTreeItem(selection).command.command, 'zucchini.selectFixture');
  assert.equal(tree.description, 'Rules');
  assert.equal(roots()[0].kind, 'rule');
  await commands.get('zucchini.find')();
  assert.deepEqual(commandCalls, ['zucchini.rules.focus', 'zucchini.rules.focus', 'list.find']);
  assert.deepEqual(revealed.map(entry => entry.node.kind), ['rule', 'requirement']);
  assert.ok(revealed.every(entry => entry.options.expand && !entry.options.select && !entry.options.focus));
  assert.equal(provider.getParent(revealed[1].node), revealed[0].node);
  await commands.get('zucchini.showScenarios')();
  assert.equal(settings.grouping, 'scenarios');
  assert.equal(tree.description, 'Scenarios');
  // Scenarios group by feature; the feature warns while a scenario does.
  const [shopFeature] = roots();
  assert.equal(shopFeature.kind, 'feature');
  assert.equal(provider.getTreeItem(shopFeature).label, 'Shop.feature', 'Without a feature name, the file names the group');
  assert.equal(provider.getTreeItem(shopFeature).description, '1 scenario · 1 with assertions not tied to a requirement');
  assert.equal(provider.getTreeItem(shopFeature).iconPath.id, 'warning');
  assert.equal(provider.getTreeItem(shopFeature).iconPath.color.id, 'list.warningForeground');
  // Scenarios expand to their highlighted lines: rule observations and unjustified assertions.
  const [buy] = provider.getChildren(shopFeature);
  assert.match(provider.getTreeItem(buy).description, /Wallet \(Debit\) · 1 not tied to a requirement/);
  assert.equal(provider.getTreeItem(buy).iconPath.id, 'warning', 'Assertions not tied to a requirement propagate to the scenario');
  assert.equal(provider.getTreeItem(buy).iconPath.color.id, 'list.warningForeground');
  const [debitLine, receiptLine] = provider.getChildren(buy);
  assert.equal(provider.getTreeItem(debitLine).label, 'Then debit');
  assert.equal(provider.getTreeItem(debitLine).description, 'Wallet (Debit)');
  assert.equal(provider.getTreeItem(debitLine).iconPath.id, 'pass');
  assert.equal(provider.getTreeItem(receiptLine).label, 'And receipt');
  assert.equal(provider.getTreeItem(receiptLine).description, "Wallet (Debit); Not tied to a requirement: 'note' in row 1");
  assert.match(provider.getTreeItem(receiptLine).tooltip, /Wallet \/ Debit \[Exact\] · coverage: item listed/);
  assert.equal(provider.getTreeItem(receiptLine).iconPath.id, 'warning', 'A step is unjustified while any of its cells is');
  const [teaRow] = provider.getChildren(receiptLine);
  assert.equal(provider.getTreeItem(teaRow).label, '| tea  |      | A1    |');
  assert.equal(provider.getTreeItem(teaRow).iconPath.id, 'warning');
  await commands.get('zucchini.open')(teaRow);
  assert.equal(shown.options.selection.line, 6);
  await commands.get('zucchini.open')(receiptLine);
  assert.equal(shown.options.selection.line, 4);
  await commands.get('zucchini.showRules')();
  assert.equal(settings.grouping, 'rules');
  assert.deepEqual(provider.getChildren(provider.getChildren(roots()[0])[0])
    .flatMap(node => provider.getChildren(node)), [], 'Rule hierarchy scenarios stay leaves');
  // Unjustified assertions are highlighted in every view, apart from observations.
  assert.equal(marks.length, 1);
  assert.equal(marks[0].line, 3);
  assert.deepEqual(assertionMarks.map(range => range.line), [4], 'Only the step line; table rows highlight cells');
  assert.deepEqual(cellMarks.covered.map(range => [range.line, range.start, range.end]), [[6, 7, 13]]);
  assert.deepEqual(cellMarks.unjustified.map(range => [range.line, range.start, range.end]), [[6, 14, 20]]);
  assert.match(hover.provideHover(doc, { line: 4 }).contents.value, /Not tied to a requirement\n'note' in row 1/);
  assert.ok(hover.provideHover(doc, { line: 4 }).contents.supportHtml, 'Strictness badges need HTML');
  assert.deepEqual(cellMarks.ignored.map(range => [range.line, range.start, range.end]), [[6, 21, 28]]);
  // Hovering a cell explains that cell alone.
  const cellHover = character => hover.provideHover(doc, { line: 6, character }).contents.value;
  // A badge sits beside the requirement, in the theme's or the configured color.
  assert.match(cellHover(9), /Wallet \/ Debit <span style="color:#1e1e1e;background-color:#cdebdc;">&nbsp;Exact&nbsp;<\/span>\ncoverage: item listed/);
  assert.doesNotMatch(cellHover(9), /Not tied|Ignored/);
  assert.match(cellHover(16), /Not tied to a requirement\n'note' in row 1/);
  settings.strictnessColors = { exact: '#102030', unspecified: 'not a color' };
  assert.match(cellHover(9), /<span style="color:#ffffff;background-color:#102030;">&nbsp;Exact&nbsp;<\/span>/);
  delete settings.strictnessColors;
  // Requirement tooltips show their strictness, unspecified included.
  const [walletRule] = roots();
  const [debitRequirement, refundRequirement] = provider.getChildren(walletRule);
  assert.match(provider.getTreeItem(debitRequirement).tooltip.value, /^Debit <span[^>]*>&nbsp;Exact&nbsp;<\/span>/);
  assert.match(provider.getTreeItem(refundRequirement).tooltip.value, /&nbsp;Unspecified&nbsp;/);
  // Statistics replace the tree with a themed page of the selected fixture.
  await commands.get('zucchini.showStatistics')();
  assert.equal(settings.grouping, 'statistics');
  const messages = [];
  const page = { webview: { cspSource: 'vscode-resource:', html: '', options: undefined,
      onDidReceiveMessage(fn) { messages.push(fn); return new Disposable(); } },
    visible: true, onDidChangeVisibility: () => new Disposable(), onDidDispose: () => new Disposable() };
  statisticsProvider.resolveWebviewView(page);
  assert.ok(page.webview.options.enableScripts);
  const html = page.webview.html;
  assert.match(html, /Selected fixture<\/div><h1>Shop<\/h1>/);
  assert.equal(page.description, 'Shop');
  assert.match(html, /These statistics are advisory only\./);
  assert.match(html, /no value, ratio or percentage among them should be used as a hard quality target/);
  assert.doesNotMatch(html, /Advisory/);
  assert.match(html, /<strong>2<\/strong><span>Asserted facts<\/span>/);
  assert.match(html, /<strong>1<\/strong><span>Tied to a requirement<\/span>/);
  assert.match(html, /No requirement explains why these assertions were made\. This could indicate a tacit requirement/);
  assert.match(html, /data-open="0" data-line="7">&#39;note&#39; in row 1</);
  assert.match(html, /\.badge\.exact\{background:#cdebdc;color:#1e1e1e\}/);
  assert.match(html, /<span class="badge exact">Exact<\/span>/);
  assert.match(html, /<span class="badge unspecified">Unspecified<\/span>/);
  assert.match(html, /script-src 'nonce-[0-9a-f]{32}'/);
  // Highlights stay on while the statistics page replaces the tree.
  tree.visible = false; visibilityListener();
  assert.equal(marks.length, 1);
  assert.ok(hover.provideHover(doc, { line: 3 }));
  tree.visible = true;
  // The page's links open assertions, the guide and the fixture picker.
  shown = undefined;
  await messages[0]({ type: 'open', scenario: 0, line: 7 });
  assert.equal(shown.options.selection.line, 6);
  await messages[0]({ type: 'help' });
  assert.deepEqual(opened, ['https://github.com/AnarchoSystems/Zucchini/blob/main/Doc/Rules.md#assertion-statistics']);
  await messages[0]({ type: 'select' });
  assert.ok(commandCalls.includes('zucchini.selectFixture'));
  // Back in the Rules view, the hidden page no longer counts as focus.
  await commands.get('zucchini.showRules')();
  page.visible = false;
  assert.equal(tree.message, undefined);
  assert.match(cellHover(23), /^Ignored: the step definition ignores this column/);
  // A report found later does not replace the shown fixture, even when it sorts first.
  const otherPath = path.join(root, 'other', 'rules.json');
  const otherFeatures = path.join(root, 'other', 'features');
  await fs.mkdir(otherFeatures, { recursive: true });
  await fs.writeFile(otherPath, JSON.stringify({ schema_version: '2.2.0', fixture: 'Other', feature_directory: otherFeatures,
    discovery_id: 'other', sources: {}, scenarios: [], rules: [] }));
  reportPaths.push(otherPath);
  await commands.get('zucchini.refresh')();
  assert.equal(provider.getTreeItem(provider.getChildren()[0]).label, 'Shop');
  // Selecting another fixture switches the tree; every fixture keeps annotating its features.
  pick = item => item.label === 'Other';
  await commands.get('zucchini.selectFixture')();
  assert.equal(workspaceState.get('zucchini.fixture'), otherPath);
  assert.equal(provider.getTreeItem(provider.getChildren()[0]).label, 'Other');
  assert.deepEqual(roots(), []);
  assert.equal(marks.length, 1);
  assert.match(hover.provideHover(doc, { line: 3 }).contents.value, /Shop · Wallet \/ Debit/);
  pick = item => item.label === 'Shop';
  await commands.get('zucchini.selectFixture')();
  pick = undefined;
  assert.equal(provider.getTreeItem(provider.getChildren()[0]).label, 'Shop');
  assert.equal(marks.length, 1);
  reportPaths.pop();
  await commands.get('zucchini.refresh')();
  const [rule] = roots();
  assert.equal(provider.getTreeItem(rule).command, undefined);
  assert.equal(provider.getTreeItem(rule).iconPath.id, 'warning');
  assert.equal(provider.getTreeItem(rule).iconPath.color.id, 'list.warningForeground');
  const [debit, refund] = provider.getChildren(rule);
  assert.equal(provider.getTreeItem(debit).iconPath.id, 'checklist', 'Covered sibling obligations do not inherit warnings');
  assert.equal(provider.getTreeItem({ ...rule, rule: { ...rule.rule, name: 'CoveredSibling', requirements: [debit.requirement] } }).iconPath.id, 'checklist', 'Covered sibling rules do not inherit warnings');
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
  const originalProcess = runner.processOutput, originalRunTests = runner.runTests;
  t.after(() => { runner.processOutput = originalProcess; runner.runTests = originalRunTests; });
  const batches = [];
  runner.processOutput = async (ctest, args) => {
    assert.ok(args.includes('--show-only=json-v1'), 'Only discovery uses processOutput directly');
    return { code: 0, output: JSON.stringify({ tests: [{ name: 'Shop.buy', command: ['ShopTests.exe', '--gtest_filter=Shop.buy'] }] }) };
  };
  runner.runTests = async options => { batches.push(options); return { code: 0, outcomes: ['passed'] }; };
  await commands.get('zucchini.run')(rule);
  assert.equal(batches.length, 1);
  assert.equal(batches[0].jobs, runner.parallelJobs());
  assert.equal(batches[0].reportDirectory, path.dirname(rule.fixture.path));
  assert.deepEqual(batches[0].tests.map(test => test.name), ['Shop.buy']);
  assert.equal(provider.getTreeItem(rule).iconPath.id, 'warning', 'Passing witnessed scenarios do not hide rule coverage gaps');
  assert.match(provider.getTreeItem(rule).description, /passed/);
  assert.equal(provider.getTreeItem(debit).iconPath.id, 'pass');
  assert.equal(provider.getTreeItem(refund).iconPath.id, 'warning');
  settings.jobs = 4;
  await commands.get('zucchini.run')(rule);
  assert.equal(batches[1].jobs, 4);
  settings.parallel = false;
  await commands.get('zucchini.run')(rule);
  assert.equal(batches[2].jobs, 1);
  delete settings.jobs; delete settings.parallel;
  runner.processOutput = originalProcess; runner.runTests = originalRunTests;
  assert.deepEqual(errors, []);
  tree.visible = false; visibilityListener();
  assert.equal(marks.length, 0);
  assert.equal(hover.provideHover(doc, { line: 3 }), undefined);
  tree.visible = true; visibilityListener();
  assert.equal(marks.length, 1);
  doc.value += '\n  Scenario: Foo\n    Then debit\n'; dirtyListener();
  await commands.get('zucchini.refresh')();
  assert.equal(marks.length, 1);
  const unassigned = roots().find(n => n.kind === 'unassigned');
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
  assert.equal(watchers.length, 4);
  doc.value = source;
  const resultPath = path.join(root, 'ctest-results.json');
  const resultWatcher = watchers.find(w => w.pattern === '**/ctest-results.json');
  const snapshot = status => JSON.stringify({ fixture: 'Shop', discovery_id: 'build-one',
    names: { 'custom Shop.buy': 'buy' },
    xml: `<testsuite><testcase name="custom Shop.buy" status="${status}"/></testsuite>` });
  async function waitFor(check) {
    for (let attempt = 0; attempt < 100; attempt++) {
      if (check()) return;
      await new Promise(resolve => setTimeout(resolve, 20));
    }
    assert.fail('Watcher did not update the view');
  }
  const currentRule = () => roots()[0];
  const currentDebit = () => provider.getChildren(currentRule())[0];
  await fs.writeFile(resultPath, snapshot('run'));
  const clearsBeforeWatcher = clearCount;
  resultWatcher.changed();
  await waitFor(() => provider.getTreeItem(currentDebit()).iconPath.id === 'pass');
  assert.equal(clearCount, clearsBeforeWatcher, 'Automatic refresh must preserve action output');
  assert.equal(provider.getTreeItem(currentDebit()).iconPath.color.id, 'testing.iconPassed');
  assert.equal(provider.getTreeItem(currentRule()).iconPath.id, 'warning');
  assert.equal(provider.getTreeItem(currentRule()).iconPath.color.id, 'list.warningForeground');
  assert.match(provider.getTreeItem(currentRule()).description, /passed/);
  await fs.writeFile(resultPath, snapshot('fail'));
  resultWatcher.changed();
  await waitFor(() => provider.getTreeItem(currentDebit()).iconPath.id === 'error');
  assert.equal(provider.getTreeItem(currentDebit()).iconPath.color.id, 'testing.iconFailed');
  assert.equal(provider.getTreeItem(currentRule()).iconPath.id, 'warning');
  assert.match(provider.getTreeItem(currentRule()).description, /failed/);
  const rebuiltReport = JSON.parse(await fs.readFile(reportPath, 'utf8'));
  rebuiltReport.discovery_id = 'build-two';
  rebuiltReport.discovery_success = false;
  await fs.writeFile(reportPath, JSON.stringify(rebuiltReport));
  watchers.find(w => w.pattern === '**/rules.json').changed();
  await waitFor(() => provider.getTreeItem(currentDebit()).iconPath.id === 'history');
  assert.equal(provider.getTreeItem(currentRule()).iconPath.id, 'warning');
  const rebuiltScenario = provider.getChildren(provider.getChildren(currentRule())[0])[0];
  // In the Rules view, warnings mean missing evidence only: the scenario's
  // assertion not tied to a requirement leaves its stale status icon alone.
  assert.equal(provider.getTreeItem(rebuiltScenario).iconPath.id, 'history');
  assert.equal(provider.getTreeItem(rebuiltScenario).description, 'Wallet (Debit)');
  assert.match(tree.message, /^Discovery failed/);
  assert.match(provider.getTreeItem(provider.getChildren()[0]).description, /discovery failed/);
  shown = undefined;
  outputLines.push('previous generation');
  await commands.get('zucchini.examples')(rule);
  assert.ok(!outputLines.includes('previous generation'));
  assert.deepEqual(errors, []);
  assert.ok(prompt.includes('Generate 2'));
  assert.match(prompt, /zucchini_read_file/, 'Generation offers source tools when the API is available');
  assert.ok(toolResults.some(text => text.includes('struct Shop {};')), 'The model can read implementation sources');
  assert.ok(docs.get(documentKey(feature)).getText().startsWith(source));
  assert.ok(docs.get(documentKey(feature)).getText().includes('Scenario: refund'));
  assert.equal(await fs.readFile(feature, 'utf8'), doc.getText());
  assert.equal(validations, 1);
  assert.deepEqual(errors, []);
  assert.equal(shown, undefined, 'Generation must not open an editor');
  assert.match(notifications.at(-1), /generation complete.*Saved and validated 1 feature file/);
  assert.ok(outputLines.some(line => line.includes('Check refunds')));
  settings.maxGenerationAttempts = 2;
  settings.fixtures = { Shop: { buildTarget: 'CustomShopTarget' } };
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

  // Find Failing Scenarios: one request per iteration proposes every scenario at once,
  // each claiming obligations; the extension confirms claims from discovery and CTest.
  applyEdits = true; doc.value = source; errors.length = 0; validations = 0;
  delete settings.fixtures;
  settings.maxFailingScenarios = 3;
  outcome = 'failed';
  const newFeature = path.join(features, 'New.feature');
  const refundEntry = { path: 'Shop.feature', content: '  Scenario: refund keeps money\n    Then debit', obligations: [{ rule: 'Wallet', obligation: 'Refund' }], rationale: 'refund never credits' };
  const debitEntry = { path: 'New.feature', content: 'Feature: new\n  Scenario: debit twice\n    Then debit', obligations: [{ rule: 'Wallet', obligation: 'Debit' }] };
  // The first reply also claims Debit, which discovery does not cover; the revision retracts it.
  reply = text => JSON.stringify(text.includes('This is iteration 2')
    ? { notes: 'Debit is enforced', scenarios: [refundEntry] }
    : { notes: 'Wallet::refund skips the credit', scenarios: [refundEntry, debitEntry] });
  prompts.length = 0;
  await commands.get('zucchini.findViolations')();
  assert.deepEqual(errors, []);
  assert.equal(prompts.length, 2, 'One request per iteration');
  assert.match(prompts[0], /^Find implementation defects in fixture Shop/);
  assert.match(prompts[0], /"obligation":"Debit".*"obligation":"Refund"/);
  assert.match(prompts[0], /at most 3 scenarios/);
  assert.match(prompts[0], /at most 2 iteration\(s\)/, 'Iterations follow zucchini.maxGenerationAttempts');
  assert.match(prompts[1], /This is iteration 2 of 2/);
  assert.match(prompts[1], /"notes":"Wallet::refund skips the credit"/);
  assert.match(prompts[1], /1\. Shop\.feature \(Wallet \/ Refund\): confirmed \(refund keeps money\)/);
  assert.match(prompts[1], /2\. New\.feature \(Wallet \/ Debit\): not confirmed: discovery recorded no coverage of Wallet \/ Debit/);
  assert.equal(validations, 2);
  assert.match(doc.getText(), /Scenario: refund keeps money/);
  assert.equal(doc.getText().match(/refund keeps money/g).length, 1, 'Revisions replace earlier additions');
  await assert.rejects(fs.access(newFeature), 'A retracted new feature is removed');
  assert.ok(outputLines.includes('Shop: 1 confirmed failing scenario(s)'));
  assert.ok(outputLines.includes('  Confirmed: Shop.feature "refund keeps money" (Wallet / Refund) — refund never credits'));
  assert.ok(outputLines.includes('Obligations with confirmed failing scenarios: Wallet / Refund (1)'));
  assert.match(notifications.at(-1), /checked 2 of 2 obligation\(s\) in 1 of 1 fixture\(s\)\. 1 confirmed failing scenario\(s\)\./);

  // An empty array ends the search without building.
  doc.value = source; prompts.length = 0; validations = 0;
  reply = () => JSON.stringify({ notes: 'Every obligation is enforced', scenarios: [] });
  await commands.get('zucchini.findViolations')();
  assert.equal(prompts.length, 1);
  assert.equal(validations, 0);
  assert.ok(outputLines.includes('Shop: no failing scenario found'));
  assert.match(notifications.at(-1), /0 confirmed failing scenario\(s\)\./);

  // The limit applies to every reply; unconfirmed scenarios after the last iteration stay for review.
  doc.value = source; prompts.length = 0; validations = 0;
  settings.maxFailingScenarios = 1;
  outcome = 'passed';
  reply = () => JSON.stringify({ notes: 'n', scenarios: [refundEntry, { ...refundEntry, content: '  Scenario: extra\n    Then debit' }] });
  await commands.get('zucchini.findViolations')();
  assert.deepEqual(errors, []);
  assert.equal(prompts.length, 2);
  assert.match(prompts[0], /at most 1 scenarios/);
  assert.ok(outputLines.includes('Ignored 1 scenario(s) beyond zucchini.maxFailingScenarios (1).'));
  assert.doesNotMatch(doc.getText(), /Scenario: extra/);
  assert.match(doc.getText(), /refund keeps money/);
  assert.ok(outputLines.includes('  Not confirmed: Shop.feature "refund keeps money" (Wallet / Refund): test passed'));
  assert.match(warnings.at(-1), /0 confirmed failing scenario\(s\), 1 unconfirmed kept for review/);
  delete settings.maxFailingScenarios;
  outcome = 'failed';

  // An unusable reply is logged and sent back once for correction.
  doc.value = source; prompts.length = 0;
  let malformed = 1;
  reply = () => malformed-- > 0 ? JSON.stringify({ notes: 'x', scenarios: 'none' }) : JSON.stringify({ notes: 'holds', scenarios: [] });
  await commands.get('zucchini.findViolations')();
  assert.deepEqual(errors, []);
  assert.equal(prompts.length, 2);
  assert.match(prompts[1], /Your previous reply could not be used: AI response must contain notes and a scenarios array/);
  assert.ok(outputLines.some(line => line.startsWith('Unusable AI reply') && line.includes('"scenarios":"none"')));

  // The view-title run covers the selected fixture; a setup error is reported per fixture.
  doc.value = source; prompts.length = 0;
  settings.fixtures = { Shop: { manifest: path.join(root, 'missing.yaml') } };
  await commands.get('zucchini.findViolations')();
  assert.equal(prompts.length, 0);
  assert.ok(outputLines.some(line => /^Shop: error: /.test(line)));
  assert.match(warnings.at(-1), /checked 0 of 2 obligation\(s\) in 0 of 1 fixture\(s\)\. 0 confirmed failing scenario\(s\), 1 fixture\(s\) not checked due to errors/);
  delete settings.fixtures;

  // A rule's search lists only its obligations; without the tool API it uses the fixed context.
  settings.sourceTools = false; doc.value = source; prompts.length = 0;
  reply = () => JSON.stringify({ notes: 'holds', scenarios: [] });
  await commands.get('zucchini.findViolations')(roots()[0]);
  assert.equal(prompts.length, 1);
  assert.match(prompts[0], /Source tools are unavailable/);
  delete settings.sourceTools;

  assert.equal(provider.getTreeItem({ kind: 'scenario', fixture: rule.fixture, scenario: rule.fixture.report.scenarios[0] }).resourceUri, undefined, 'Scenario rows must not inherit file-level Git decorations');
  assert.equal(commands.has('zucchini.writeRule'), false);
});
