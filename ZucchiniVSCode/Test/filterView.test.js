'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const vm = require('node:vm');
const { FixtureFilterView } = require('../src/filterView');

test('sidebar input filters as typed, clears with Escape, and syncs external settings', async () => {
  let receive, timer, incoming, current = 'Shop';
  const view = { webview: {
    onDidReceiveMessage(fn) { receive = fn; return { dispose() {} }; },
    postMessage(message) { incoming({ data: message }); }
  } };
  const provider = new FixtureFilterView(() => current, async value => { current = value; });
  provider.resolveWebviewView(view);
  const handlers = {}, input = { value: '', addEventListener(name, fn) { handlers[name] = fn; }, focus() { document.activeElement = input; } };
  const clear = { addEventListener(name, fn) { handlers.clear = fn; } };
  const document = { activeElement: undefined, getElementById: id => id === 'filter' ? input : clear };
  vm.runInNewContext(view.webview.html.match(/<script nonce="[^"]+">([\s\S]*?)<\/script>/)[1], {
    document, window: { addEventListener(_name, fn) { incoming = fn; } },
    acquireVsCodeApi: () => ({ postMessage: receive }),
    setTimeout: fn => { timer = fn; return 1; }, clearTimeout: () => { timer = undefined; }
  });
  assert.equal(input.value, 'Shop');
  input.focus(); input.value = '!Legacy*'; handlers.input(); timer();
  await provider.pending;
  assert.equal(current, '!Legacy*');
  handlers.keydown({ key: 'Escape', preventDefault() {} });
  await provider.pending;
  assert.equal(current, '');
  assert.equal(input.value, '');
  input.value = 'Other'; handlers.clear(); await provider.pending;
  assert.equal(current, '');
  document.activeElement = undefined;
  provider.update('Inventory');
  assert.equal(input.value, 'Inventory');
  provider.dispose();
});
