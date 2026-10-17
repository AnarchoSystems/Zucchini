'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { requestDraft, supportsDraftRequests } = require('../src/generation');

test('excludes Copilot CLI chat-session models while retaining direct providers', () => {
  const models = [{ vendor: 'copilotcli', id: 'auto' }, { vendor: 'copilotcli', id: 'sonnet' },
    { vendor: 'copilot', id: 'sonnet' }, { vendor: 'custom', id: 'local' }];
  assert.deepEqual(models.filter(supportsDraftRequests), models.slice(2));
});

test('rejects Copilot CLI before counting tokens or sending a request', async () => {
  const state = setup([]);
  state.model.vendor = 'copilotcli';
  state.model.countTokens = () => assert.fail('Should not count tokens');
  await assert.rejects(state.run(), /only support chat sessions/);
  assert.equal(state.requests.length, 0);
});

function setup(replies, token = { isCancellationRequested: false }) {
  const requests = [], logs = [], progress = [];
  const model = {
    name: 'Sonnet', vendor: 'copilot', id: 'sonnet', maxInputTokens: 10000,
    countTokens: async () => 100,
    async sendRequest(messages, options, cancellation) {
      requests.push(messages);
      assert.equal(cancellation, token);
      const reply = replies.shift();
      return { text: (async function* () {
        for (const part of reply) {
          if (part instanceof Error) throw part;
          if (typeof part === 'function') part();
          else yield part;
        }
      })() };
    }
  };
  return { requests, logs, progress, model, token,
    run: () => requestDraft(model, 'fixture context', token, {
      message: text => ({ text }), log: text => logs.push(text), progress: text => progress.push(text)
    }) };
}

test('collects streamed text without retrying or logging source and response content', async () => {
  const state = setup([['{"explanation":', '"ok","additions":[]}']]);
  assert.equal(await state.run(), '{"explanation":"ok","additions":[]}');
  assert.equal(state.requests.length, 1);
  assert.equal(state.progress.length, 0);
  assert.match(state.logs[0], /response request 1\/2/);
  assert.doesNotMatch(state.logs[0], /attempt 1\/2/);
  assert.ok(state.logs.every(line => !line.includes('fixture context') && !line.includes('explanation')));
});

test('retries an empty response once with an explicit final text instruction', async () => {
  const state = setup([[' ', '\n'], ['{"explanation":"ok","additions":[]}']]);
  assert.match(await state.run(), /explanation/);
  assert.equal(state.requests.length, 2);
  assert.match(state.requests[1][0].text, /Return the requested JSON object as your final text response/);
  assert.equal(state.progress.length, 1);
});

test('persistent empty responses identify the provider and stop after two attempts', async () => {
  const state = setup([[], []]);
  await assert.rejects(state.run(), /Sonnet \(copilot \/ sonnet\) returned an empty draft twice/);
  assert.equal(state.requests.length, 2);
});

test('cancellation during an empty stream exits without retrying', async () => {
  const state = setup([[() => { state.token.isCancellationRequested = true; }]]);
  assert.equal(await state.run(), undefined);
  assert.equal(state.requests.length, 1);
});

test('stream failures preserve the provider error without retrying partial output', async () => {
  const error = Object.assign(new Error('quota exceeded'), { code: 'quota' });
  const state = setup([['partial', error]]);
  await assert.rejects(state.run(), error);
  assert.equal(state.requests.length, 1);
  assert.match(state.logs.at(-1), /quota: quota exceeded/);
});

test('oversized input is rejected before sending', async () => {
  const state = setup([]);
  state.model.countTokens = async () => 9500;
  await assert.rejects(state.run(), /context is too large/);
  assert.equal(state.requests.length, 0);
});

// Tool-calling conversations: parts are { text } or { call: { callId, name, input } }.
function toolSetup(rounds, { maxCalls = 5, reject } = {}) {
  const requests = [], logs = [], progress = [], invoked = [];
  const token = { isCancellationRequested: false };
  const chat = {
    text: part => part.text,
    toolCall: part => part.call,
    assistant: parts => ({ role: 'assistant', parts }),
    results: results => ({ role: 'tool', results })
  };
  const tools = {
    definitions: [{ name: 'zucchini_read_file', description: 'read', inputSchema: { type: 'object' } }],
    maxCalls, chat,
    async invoke(name, input) { invoked.push({ name, input }); return `contents of ${input.path}`; }
  };
  const model = {
    name: 'Sonnet', vendor: 'copilot', id: 'sonnet', maxInputTokens: 10000,
    countTokens: async () => 100,
    async sendRequest(messages, options) {
      requests.push({ messages: [...messages], options });
      if (reject && options.tools) throw Object.assign(new Error('Tool calling is not supported by this model'), { code: 'NoToolSupport' });
      const parts = rounds.shift();
      return { stream: (async function* () { yield* parts; })(), text: (async function* () { for (const part of parts) if (part.text) yield part.text; })() };
    }
  };
  return { requests, logs, progress, invoked, model, token, tools,
    run: () => requestDraft(model, 'fixture context', token, {
      message: text => ({ role: 'user', text }), log: text => logs.push(text), progress: text => progress.push(text), tools
    }) };
}

test('the model can read sources with tools before answering', async () => {
  const state = toolSetup([
    [{ text: 'Let me look.' }, { call: { callId: 'a', name: 'zucchini_read_file', input: { path: 'src/Shop.cpp' } } }],
    [{ text: '{"explanation":"off by one","additions":[]}' }]
  ]);
  assert.equal(await state.run(), '{"explanation":"off by one","additions":[]}');
  assert.deepEqual(state.invoked, [{ name: 'zucchini_read_file', input: { path: 'src/Shop.cpp' } }]);
  assert.equal(state.requests.length, 2);
  assert.equal(state.requests[0].options.tools, state.tools.definitions);
  const [user, assistant, results] = state.requests[1].messages;
  assert.equal(user.text, 'fixture context');
  assert.equal(assistant.parts.length, 2, 'Assistant turn keeps its text and tool calls');
  assert.deepEqual(results.results, [{ callId: 'a', text: 'contents of src/Shop.cpp' }]);
  assert.ok(state.logs.some(line => /Source tool 1\/5: zucchini_read_file src\/Shop\.cpp/.test(line)));
  assert.ok(state.logs.every(line => !line.includes('contents of')), 'Tool output is not logged');
});

test('an exhausted tool budget withdraws the tools and asks for the answer', async () => {
  const call = id => ({ call: { callId: id, name: 'zucchini_read_file', input: { path: id } } });
  const state = toolSetup([[call('a'), call('b'), call('c')], [{ text: '{"explanation":"x","additions":[]}' }]], { maxCalls: 2 });
  assert.match(await state.run(), /explanation/);
  assert.equal(state.invoked.length, 2);
  const second = state.requests[1];
  assert.equal(second.options.tools, undefined);
  assert.match(second.messages[2].results[2].text, /budget exhausted/);
  assert.match(second.messages.at(-1).text, /tool budget is exhausted/);
});

test('providers that reject tools fall back to the fixed context', async () => {
  const state = toolSetup([[{ text: '{"explanation":"x","additions":[]}' }]], { reject: true });
  assert.match(await state.run(), /explanation/);
  assert.equal(state.requests.length, 2);
  assert.equal(state.requests[1].options.tools, undefined);
  assert.ok(state.logs.some(line => /rejected source tools/.test(line)));
});

test('tool exploration that outgrows the context window stops with guidance', async () => {
  const state = toolSetup([[{ call: { callId: 'a', name: 'zucchini_read_file', input: { path: 'big' } } }]]);
  let calls = 0;
  state.model.countTokens = async () => (++calls > 1 ? 5000 : 100);
  await assert.rejects(state.run(), /Source exploration exceeded the context window.*maxSourceToolCalls/);
});
