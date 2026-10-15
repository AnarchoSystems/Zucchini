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
  "zucchini.reportPatterns": ["**/zucchini-reports/**/rules.json"],
  "zucchini.ctestPath": "ctest",
  "zucchini.fixtures": {
    "Shop": {
      "buildDirectory": "/absolute/path/to/build",
      "configuration": "Debug",
      "manifest": "/absolute/path/to/features/steps.yaml",
      "sourceFiles": ["/absolute/path/to/Shop.h", "/absolute/path/to/Shop.cpp"]
    }
  }
}
```

All fixture overrides are optional. Keys can also be absolute report paths to
separate fixtures/configurations that share a name. By default the build directory
is the closest ancestor containing `CMakeCache.txt`; standard configuration names
are inferred from the report parent directory. Specify `configuration` for custom
Visual Studio configurations or reports outside the normal layout. Windows paths
can use forward slashes or JSON-escaped backslashes. Process arguments are passed
without a shell.

For scenario generation, the manifest defaults to the sole YAML file directly under `feature_directory`.
If that directory also has a stylesheet, set `manifest` explicitly. Default source
context starts at `<feature_directory>/../<fixture>.h` (or `.hpp`) and follows
quoted includes that exist alongside their including file. Configure `sourceFiles`
when this is insufficient. Standard/library and generated includes are not
searched through compiler include paths.

## Validation

```sh
npm ci
npm test
npm run check
```

Tests cover grouping switches, fixture filtering, output resets, inline filter updates, missing witnesses, scenario deduplication, Windows
source paths, CTest filtering, prompt content, fixture context, and restricted
feature writes. Manual Extension Host validation is still needed for tree layout,
theme appearance, sidebar filter sizing, and a real provider's authorization and generated output.
