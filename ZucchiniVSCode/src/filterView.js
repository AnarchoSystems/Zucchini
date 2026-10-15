'use strict';
const { randomBytes } = require('node:crypto');

class FixtureFilterView {
  constructor(value, change) {
    this.value = value;
    this.change = change;
    this.pending = Promise.resolve();
  }
  resolveWebviewView(view) {
    this.listener?.dispose();
    this.view = view;
    const nonce = randomBytes(16).toString('hex');
    view.webview.options = { enableScripts: true, localResourceRoots: [] };
    this.listener = view.webview.onDidReceiveMessage(message => {
      if (message?.type === 'ready') this.update(this.value());
      if (message?.type === 'filter' && typeof message.value === 'string') {
        this.pending = this.pending.then(() => this.change(message.value));
      }
      return this.pending;
    });
    view.webview.html = `<!DOCTYPE html>
<html><head><meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'nonce-${nonce}'; script-src 'nonce-${nonce}';">
<style nonce="${nonce}">
body { padding: 6px 12px; margin: 0; color: var(--vscode-foreground); font-family: var(--vscode-font-family); }
.field { display: flex; align-items: center; background: var(--vscode-input-background); border: 1px solid var(--vscode-input-border, transparent); }
.field:focus-within { border-color: var(--vscode-focusBorder); }
input { width: 100%; min-width: 0; box-sizing: border-box; padding: 5px; border: 0; outline: none; background: transparent; color: var(--vscode-input-foreground); font: inherit; }
input::placeholder { color: var(--vscode-input-placeholderForeground); }
button { background: transparent; border: 0; color: var(--vscode-foreground); cursor: pointer; font-size: 18px; }
button:focus-visible { outline: 1px solid var(--vscode-focusBorder); }
</style></head><body>
<div class="field"><input id="filter" type="text" aria-label="Filter fixtures" placeholder="Filter fixtures" title="Fixture names: comma-separated includes, !excludes, * and ? wildcards" autocomplete="off" spellcheck="false"><button id="clear" aria-label="Clear fixture filter" title="Clear fixture filter (Escape)">×</button></div>
<script nonce="${nonce}">
const vscode = acquireVsCodeApi();
const input = document.getElementById('filter');
let timer;
function send() { clearTimeout(timer); vscode.postMessage({type: 'filter', value: input.value}); }
function clear() { input.value = ''; send(); input.focus(); }
input.addEventListener('input', () => { clearTimeout(timer); timer = setTimeout(send, 150); });
input.addEventListener('keydown', event => { if (event.key === 'Escape') { event.preventDefault(); clear(); } if (event.key === 'Enter') send(); });
document.getElementById('clear').addEventListener('click', clear);
window.addEventListener('message', event => {
  if (event.data.type === 'value' && document.activeElement !== input) input.value = event.data.value;
});
vscode.postMessage({type: 'ready'});
</script></body></html>`;
  }
  update(value) { this.view?.webview.postMessage({ type: 'value', value }); }
  dispose() { this.listener?.dispose(); }
}
module.exports = { FixtureFilterView };
