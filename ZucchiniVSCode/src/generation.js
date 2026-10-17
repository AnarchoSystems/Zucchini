'use strict';

// copilotcli registers models for chat sessions, but its direct response handler
// is a no-op. These models cannot serve LanguageModelChat.sendRequest callers.
function supportsDraftRequests(model) {
  return model.vendor !== 'copilotcli';
}

// Streams a response into text and tool calls. `chat.text` / `chat.toolCall` map a
// streamed part to its text or to { callId, name, input }; providers without a
// part stream fall back to the plain text stream.
async function collect(response, chat, token) {
  const result = { text: '', parts: [], calls: [], fragments: 0 };
  if (!response.stream || !chat) {
    for await (const fragment of response.text) {
      if (token.isCancellationRequested) return;
      result.fragments++;
      result.text += fragment;
    }
    return result;
  }
  for await (const part of response.stream) {
    if (token.isCancellationRequested) return;
    const call = chat.toolCall(part);
    if (call) { result.calls.push(call); result.parts.push(part); continue; }
    const text = chat.text(part);
    if (typeof text === 'string') { result.fragments++; result.text += text; result.parts.push(part); }
  }
  return result;
}

function describe(call) {
  const input = call.input && typeof call.input === 'object' ? call.input : {};
  const target = input.path || input.query || '';
  return `${call.name}${target ? ` ${String(target).slice(0, 120)}` : ''}`;
}

// Requests a draft from the model. With `tools`, the model may call workspace
// source tools before answering; each round's assistant turn and tool results are
// appended to the conversation. `tools.chat` adapts the provider's message types.
async function requestDraft(model, prompt, token, { message, log, progress, tools, purpose = 'Scenario generation' }) {
  if (!supportsDraftRequests(model)) throw new Error('Copilot CLI models only support chat sessions. Select a model from the copilot provider or another language model provider.');
  const label = `${model.name} (${model.vendor} / ${model.id})`;
  let useTools = Boolean(tools?.definitions?.length);
  const maxCalls = Math.max(0, Math.trunc(tools?.maxCalls ?? 40));
  for (let attempt = 1; attempt <= 2; attempt++) {
    if (token.isCancellationRequested) return;
    const messages = [message(prompt + (attempt === 2
      ? `\nYour previous request returned no draft text. Return the requested JSON object as your final text response.${useTools ? '' : ' Do not call tools.'} If scenarios cannot be generated, return an explanation and an empty additions array.`
      : ''))];
    const counts = [];
    const started = Date.now();
    let calls = 0;
    let offerTools = useTools && maxCalls > 0;
    for (let round = 0; ; round++) {
      for (let index = counts.length; index < messages.length; index++) {
        counts.push(await model.countTokens(messages[index], token));
        if (token.isCancellationRequested) return;
      }
      const tokens = counts.reduce((sum, count) => sum + count, 0);
      if (tokens > model.maxInputTokens * 0.9) {
        if (!calls) throw new Error('Fixture context is too large for this model. Select a model with a larger context window.');
        throw new Error(`Source exploration exceeded the context window of ${label} after ${calls} tool call(s). Select a model with a larger context window, lower zucchini.maxSourceToolCalls, or narrow zucchini.fixtures sourceFiles.`);
      }
      log(`${purpose}: ${label}, response request ${attempt}/2 (one retry for empty responses)${round ? `, tool round ${round}` : ''}, input tokens ${tokens}/${model.maxInputTokens}.`);
      let streamed;
      try {
        const response = await model.sendRequest(messages, offerTools ? { tools: tools.definitions } : {}, token);
        streamed = await collect(response, useTools ? tools.chat : undefined, token);
      } catch (error) {
        if (token.isCancellationRequested) return;
        // Some providers reject tool definitions outright; continue with the fixed context.
        if (offerTools && !calls && /tool/i.test(`${error.code || ''} ${error.message}`)) {
          log(`${purpose}: ${label} rejected source tools (${error.code || error.name}: ${error.message}). Continuing without them.`);
          useTools = offerTools = false;
          round--;
          continue;
        }
        log(`${purpose} failed for ${label}: ${error.code || error.name}: ${error.message}`);
        throw error;
      }
      if (!streamed) return;
      if (streamed.calls.length) {
        if (round > maxCalls + 2) throw new Error(`${label} kept requesting source tools after the budget was exhausted.`);
        messages.push(tools.chat.assistant(streamed.parts));
        const results = [];
        for (const call of streamed.calls) {
          if (token.isCancellationRequested) return;
          calls++;
          if (calls > maxCalls) { results.push({ callId: call.callId, text: 'Tool budget exhausted. Answer now with the information you have.' }); continue; }
          log(`Source tool ${calls}/${maxCalls}: ${describe(call)}`);
          progress(`Reading sources (${calls} tool call${calls === 1 ? '' : 's'})…`);
          results.push({ callId: call.callId, text: await tools.invoke(call.name, call.input) });
        }
        messages.push(tools.chat.results(results));
        if (calls >= maxCalls) {
          offerTools = false;
          messages.push(message('The source tool budget is exhausted. Return the requested JSON object now as your final text response.'));
        }
        continue;
      }
      log(`${purpose}: ${streamed.fragments} text fragments, ${streamed.text.length} characters${calls ? `, ${calls} source tool call(s)` : ''}, ${Math.round((Date.now() - started) / 1000)}s.`);
      if (streamed.text.trim()) return streamed.text;
      break;
    }
    if (attempt === 1) progress('The model returned no text. Retrying once…');
  }
  throw new Error(`${label} returned an empty draft twice. Try another model, or check the provider's output token limit and logs. See the Zucchini output channel for request diagnostics.`);
}

module.exports = { requestDraft, supportsDraftRequests };
