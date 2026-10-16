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
