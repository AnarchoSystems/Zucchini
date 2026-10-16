# Zucchini Rules for VS Code

Explore your [Zucchini](https://github.com/AnarchoSystems/Zucchini) business rules,
see which scenarios cover them, and run
those scenarios without leaving your editor. When coverage is missing, ask AI
to generate passing, failing, or exploratory scenarios.

- **Find coverage gaps.** Browse rules and their obligations to see what still
  needs an example.
- **Read scenarios in context.** Jump to a scenario and hover over highlighted
  steps to see the rules they exercise.
- **Run what matters.** Run one scenario, a rule’s scenarios, or a whole fixture,
  and see the results in the sidebar.
- **Draft more scenarios.** Generate passing, failing, or exploratory scenarios using
  a language model available in VS Code.

## Get started

1. Install **Zucchini Rules** from the VS Code Extensions Marketplace.
2. Open your Zucchini project. Enable `RULE_COVERAGE_REPORT` in your project’s
   `zucchinify()` call and build it to generate the rule reports.
3. Click the **Zucchini** icon in the Activity Bar to explore your rules.

Requires VS Code 1.95 or newer. To run scenarios, CTest must be available on your
PATH, or configured in the extension’s settings. Build your project with your
usual tools; the view updates when reports change.

For help setting up reports, see the
[rules setup guide](https://github.com/AnarchoSystems/Zucchini/blob/main/Doc/Rules.md#enable-the-report).

## Explore your rules

Expand a fixture to see its rules, then expand a rule to see its obligations and
the scenarios that exercise them. Warning icons flag obligations without coverage.
Use **Show Flat Scenarios** in the view toolbar to list scenarios directly under
each fixture. The same button becomes **Show Rule Hierarchy** to switch back;
the view description shows the current mode.

Use the funnel button in the view toolbar to open VS Code's native Find field
inside the tree. Its filter toggle switches between highlighting matches and
hiding nonmatching rows. The funnel expands and loads branches first so the native filter can evaluate
the nested hierarchy. This is a tree label search, not the fixture-pattern filter. Press Escape to close the field.
For persistent fixture-name patterns, use **Zucchini: Fixture Filter** in Settings
(for example `Shop*, Inventory, !Legacy*`).

Click a scenario to open its feature file. While the Zucchini view is visible,
steps with rule observations are highlighted. Hover over them for details.

Coverage tells you that a scenario exercised an obligation during discovery.
Run the scenario to check whether it passes.

## Run scenarios

Click the play button beside a scenario, obligation, rule, or fixture. Results
appear in the sidebar; detailed output is available in the **Zucchini** output
channel. Starting a run, AI action, or manual refresh clears the previous output.
The view also picks up results from runs outside the extension.

After you edit a scenario, save and rebuild your project before running it here.
New scenarios appear as **Awaiting discovery** until then. Older results are
marked stale when they no longer match the current scenarios.

## Generate scenarios with AI

Click the sparkle button beside a rule or obligation and choose:

- **Generate Passing Examples** to demonstrate the expected behavior.
- **Generate Failing Counterexamples** to look for cases where the implementation
  breaks the rule.

Choose a model when prompted. You’ll need a language model provider in VS Code
that supports extension requests, such as GitHub Copilot Chat. Authentication
and any charges are handled by your provider.

Generation sends the selected rule, feature files, step definitions, and relevant
implementation source to your chosen provider, from the editor. Save fixture context before generating so validation uses the same source.
This context helps the model use your existing steps and understand the behavior
it should test.

Generated additions are saved to feature files. The extension builds with CMake,
rediscovers tests, and runs the new scenarios with CTest. Passing scenarios should
pass; failing scenarios must expose implementation defects against the business rules and should fail
against the declared requirements. Exploratory scenarios exercise behavior
without requiring either outcome. Discovery rejection and build errors do not count
as intended failures.

Build and test diagnostics go back to the model for revision when the requested
outcome is not reached. Set `zucchini.maxGenerationAttempts` in `settings.json`
to control the attempt limit (default 3, range 1–20). Revisions replace only additions
from the current generation. Saved scenarios remain available if validation fails or
you cancel. Review their assertions and step usage even after validation succeeds.
The model can also explain that it cannot produce suitable scenarios.

Progress and completion appear as notifications. The model’s explanation is
available in the **Zucchini** output channel. Running scenarios and generating
with AI require a trusted workspace.

## Settings and help

Search for **Zucchini** in VS Code Settings to change the scenario count, report
locations, grouping, or CTest path. Most projects can use the defaults.

- **No rules appear?** Build your project with rule coverage reports enabled,
  then run **Zucchini: Refresh Reports**. If reports are in a custom location,
  update **Zucchini: Report Patterns**.
- **A report is incompatible?** Update the extension or rebuild with a compatible
  Zucchini version.
- **Generation cannot find your sources?** Set the fixture’s `manifest` or
  `sourceFiles` in **Zucchini: Fixtures**.
- **No usable AI model appears?** Enable GitHub Copilot Chat or another provider
  that supports extension requests. Copilot CLI chat-session models cannot be
  used for scenario generation.

See the [configuration and development guide](https://github.com/AnarchoSystems/Zucchini/blob/main/ZucchiniVSCode/DEVELOPMENT.md)
for advanced settings and contributor instructions. Report problems on
[GitHub](https://github.com/AnarchoSystems/Zucchini/issues).

Scenario runs and generation validation send the entire selection to one CTest
invocation. CTest runs with all processors available to the extension host by
default, while respecting test resource locks and scheduling properties.
Override the worker count or disable parallel execution in `settings.json`:

```json
{
  "zucchini.jobs": 4,
  "zucchini.parallel": true
}
```

`zucchini.jobs: 0` (the default) detects the available processor count.
`zucchini.parallel: false` forces `-j1`, regardless of the jobs setting.
