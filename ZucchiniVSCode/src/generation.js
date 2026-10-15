'use strict';

// copilotcli registers models for chat sessions, but its direct response handler
// is a no-op. These models cannot serve LanguageModelChat.sendRequest callers.
function supportsDraftRequests(model) {
  return model.vendor !== 'copilotcli';
}

async function requestDraft(model, prompt, token, { message, log, progress }) {
  if (!supportsDraftRequests(model)) throw new Error('Copilot CLI models only support chat sessions. Select a model from the copilot provider or another language model provider.');
  const label = `${model.name} (${model.vendor} / ${model.id})`;
  for (let attempt = 1; attempt <= 2; attempt++) {
    if (token.isCancellationRequested) return;
    const request = message(prompt + (attempt === 2
      ? '\nYour previous request returned no draft text. Return the requested JSON object as your final text response. Do not call tools. If scenarios cannot be generated, return an explanation and an empty additions array.'
      : ''));
    const tokens = await model.countTokens(request, token);
    if (token.isCancellationRequested) return;
    if (tokens > model.maxInputTokens * 0.9) throw new Error('Fixture context is too large for this model. Select a model with a larger context window.');
    log(`Scenario generation: ${label}, attempt ${attempt}/2, input tokens ${tokens}/${model.maxInputTokens}.`);
    let draft = '';
    let fragments = 0;
    try {
      const response = await model.sendRequest([request], {}, token);
      for await (const fragment of response.text) {
        if (token.isCancellationRequested) return;
        fragments++;
        draft += fragment;
      }
    } catch (error) {
      if (token.isCancellationRequested) return;
      log(`Scenario generation failed for ${label}: ${error.code || error.name}: ${error.message}`);
      throw error;
    }
    if (token.isCancellationRequested) return;
    log(`Scenario generation: ${fragments} text fragments, ${draft.length} characters.`);
    if (draft.trim()) return draft;
    if (attempt === 1) progress('The model returned no text. Retrying once…');
  }
  throw new Error(`${label} returned an empty draft twice. Try another model, or check the provider's output token limit and logs. See the Zucchini output channel for request diagnostics.`);
}

module.exports = { requestDraft, supportsDraftRequests };
