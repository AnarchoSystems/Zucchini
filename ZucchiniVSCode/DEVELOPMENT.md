# Zucchini Rules: configuration and development

## Report compatibility

The extension accepts report schema versions `>=2.0.0 <3.0.0`, as declared in
`package.json` under `zucchiniRuleReport.versionRange`. See the
[report schema and versioning policy](../schemas/README.md) for the format and
compatibility rules. Reports with an unsupported version show a compatibility
message; rebuild with a compatible Zucchini version or update the extension.
Reports in ignored build directories are included in searches.

## Local development and packaging

Use Node.js 20 or newer. From `ZucchiniVSCode`, run `npm ci` to install
dependencies. The extension runs directly from JavaScript; no compile step is
required. To debug it, configure a VS Code extension launch with this directory
as the extension development path and open a Zucchini project in the Extension
Development Host.

To create an installer, run `npx @vscode/vsce package` in this directory and use
**Extensions: Install from VSIX**. After replacing an installed build, run
**Developer: Reload Window**, including when reinstalling the same version.
Rebuilding fixtures does not reload the extension host.

## Settings

```json
{
  "zucchini.grouping": "rules",
  "zucchini.fixtureFilter": "",
  "zucchini.scenarioCount": 3,
  "zucchini.maxFailingScenarios": 5,
  "zucchini.sourceTools": true,
  "zucchini.maxSourceToolCalls": 40,
  "zucchini.reportPatterns": ["**/zucchini-reports/**/rules.json"],
  "zucchini.ctestPath": "ctest",
  "zucchini.parallel": true,
  "zucchini.jobs": 0,
  "zucchini.fixtures": {
    "Shop": {
      "buildDirectory": "/absolute/path/to/build",
      "buildTarget": "ShopTests",
      "configuration": "Debug",
      "manifest": "/absolute/path/to/features/steps.yaml",
      "sourceFiles": ["/absolute/path/to/Shop.h", "/absolute/path/to/Shop.cpp"]
    }
  }
}
```

All fixture overrides are optional. Keys can also be `executable/fixture` or absolute report paths to
separate fixtures/configurations that share a name. By default the build directory
is the closest ancestor containing `CMakeCache.txt`; standard configuration names
are inferred from the report parent directory. Specify `configuration` for custom
Visual Studio configurations or reports outside the normal layout. Windows paths
can use forward slashes or JSON-escaped backslashes. Process arguments are passed
without a shell.

Generation validation builds only `buildTarget` and its dependencies, never the
default/all target. It defaults to the report's executable basename with a Windows
`.exe` suffix removed. Override it when the CMake target differs from the output
filename (such as with `OUTPUT_NAME`, `DEBUG_POSTFIX`, or an executable suffix),
or when using an older report without `executable`. The target is checked before
requesting AI additions. Builds reuse the existing directory and configuration;
they do not configure a fresh build tree or request a clean rebuild.

For scenario generation, the manifest defaults to the sole YAML file directly under `feature_directory`.
If that directory also has a stylesheet, set `manifest` explicitly. Default source
context starts at `<feature_directory>/../<fixture>.h` (or `.hpp`) and follows
quoted includes that exist alongside their including file. Implementation files next
to those headers (same name with `.cpp`, `.cc`, `.cxx`, `.c`, `.ipp`, `.inl` or `.tpp`)
are named in the prompt but not inlined. Standard/library and generated includes are not
searched through compiler include paths. Configure `sourceFiles` to inline more.

When the selected model supports tool calling, generation also offers three read-only
tools: `zucchini_list_files`, `zucchini_search` and `zucchini_read_file`. They resolve paths
relative to the workspace folders and refuse anything outside them, including through
symbolic links. They skip `.git`, `node_modules`, `CMakeFiles` and similar folders, skip
binary files and files over 2 MB, and cap listings, matches and read sizes. Reads come
from disk, which the build also uses, and note when an editor has unsaved changes. With
tools available, a missing default fixture header is reported to the model instead of
failing generation. If the provider rejects tool definitions, generation continues with
the fixed context. Requests stop when the conversation exceeds 90% of the model's input
window; lower `maxSourceToolCalls` or choose a larger model if that happens.

**Find Failing Scenarios for All Obligations** (`zucchini.findViolations`) enumerates
business-rule obligations in report order, targeting a rule as a whole when it declares
none, and searches each fixture separately (`huntFixture` in `extension.js`, with the
prompt, parsing, planning and verification in `hunt.js`). Each iteration is one request
(`huntPrompt`, budget `maxSourceToolCalls`), up to `maxGenerationAttempts` iterations. The
context also inlines the implementation files beside included headers (`collectContext`'s
`inlineRelated`) to save tool round trips, and the prompt asks for batched tool calls.

The reply is `{notes, scenarios: [{path, content, obligations: [{rule, obligation}],
rationale}]}` (`parseHunt`). Entries beyond `maxFailingScenarios` are dropped and
logged. `planHunt` appends the entries for each path in order, relative to the original
file text. It records each entry's line range, so discovered scenarios map back to
entries without relying on names. Files written by an earlier iteration that the new
list no longer uses are restored, or deleted if that iteration created them.

`validateGeneration` returns each new scenario's outcome (`rejected` and `unregistered`
included) and the rebuilt report. `verifyHunt` confirms an entry when it has at least
one claim and every scenario discovered in its lines is accepted, has coverage events
for every claimed obligation, draws no violation event from any rule (DSL rules
included), and failed. A whole-rule claim matches any coverage of that rule, including
when the claim repeats the rule name as the obligation. Another iteration runs only
while an entry is unconfirmed. `huntRevision` then appends the previous notes and
entries, the per-entry verdicts, and the tail of the diagnostics to the initial prompt.
An empty list ends the fixture. Any error ends that fixture, and the run continues.

Replies may wrap the JSON object in fences or prose, and a list or object `explanation`
is joined into text. Omitted `additions` (or, for failing-scenario searches, an omitted
`scenarios` list beside `notes`) count as none. Other malformed replies and
rejected feature paths are marked `invalidReply`. They are written to the output
channel and sent back to the model once for correction, before the attempt fails.

## Validation

```sh
npm ci
npm test
npm run check
```

Tests cover grouping switches, fixture filtering, output resets, native tree filtering, missing witnesses, scenario deduplication, Windows
source paths, CTest filtering, prompt content, fixture context, source tools and the
tool-calling loop, failing-scenario searches and their verification, and restricted feature writes. Manual Extension Host validation is still needed for tree layout,
theme appearance, native tree Find/Filter focus and keyboard navigation, and a real provider's authorization and generated output.
