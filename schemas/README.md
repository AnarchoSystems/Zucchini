# JSON schemas

- `schema.json`: step-definition manifests.
- `stylesheet.schema.json`: generator stylesheets.
- `rules-report.v2.0.1.schema.json`: current rule coverage reports (`rules.json`).

## Rule report versions

`schema_version` is a required SemVer string, independent of the Zucchini release
and extension version. Current reports emit `"schema_version": "2.0.1"` and a
`$schema` reference to the matching versioned schema. The reference is optional;
the version field is authoritative.

`ZucchiniRuntime/CMakeLists.txt` selects the current schema. CMake reads its
`schema_version` constant and `$id` to generate the runtime version header. The
report writer owns these two fields and stamps them on every emitted report,
including reports of failed discovery. The generator and scenario fixture use
the same runtime version API. The generated header is private to the runtime,
so consumers need no generated include path.

The extension declares its supported SemVer range in `package.json` under
`zucchiniRuleReport.versionRange`, currently `>=2.0.0 <3.0.0`. The standard `semver`
package evaluates compatibility. Missing, numeric, abbreviated and malformed
versions are rejected with an explanation of the supported range. Stable minor
and patch releases within the range are accepted. Build metadata does not change
compatibility; prereleases are excluded unless the range explicitly opts in.
Tests check that the selected runtime schema falls within the extension's range.

Keep older versioned schema files when introducing a new format. Breaking
changes (removed/renamed fields, changed field types or meanings, new required
fields, or new closed-enum values) require a major-version bump. Compatible new
optional fields require a minor-version bump. Clarifications and fixes that do
not change the format use a patch-version bump. Create the corresponding schema
filename and `$id`, select it in the runtime CMake configuration, and check the
extension's compatibility range. Do not change a published schema in a way that
invalidates its existing reports. Unknown fields are allowed so compatible
additions remain readable by older consumers within the same major version.

The schema describes discovery declarations, diagnostic events, scenarios,
resolved step arguments, source snapshots and discovery identity. Empty rule
and scenario lists are valid, including after failed discovery. Unwitnessed
requirements remain declarations with no matching coverage event. Per-scenario
runtime result files under `results/` are a separate format and are not covered
by this schema. Cross-record identities (such as an event's `scenario_id`) are
semantic constraints rather than JSON Schema validation rules.

Version 2.0.1 fixes fixture identity: generated reports include `executable`
(the executable filename, including any platform suffix), and `fixture` is the
raw user-specified name. Fixture names are scoped to an executable. The extension
shows `executable/fixture` for duplicate fixture names without adding a tree level.
The executable field remains optional when reading older reports.
