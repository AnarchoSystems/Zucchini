# Example Coverage

The examples are executable proof of Zucchini's generator, discovery, and execution boundaries.

- `Good` contains successful end-to-end scenarios.
- `Good/PreTestDiscovery` is a standalone `zucchinify()` project using `DISCOVERY_MODE PRE_TEST`; `CMakeIntegration.PreTestDiscovery` verifies plans are created during CTest pre-discovery, not while building.
- `Bad` contains standalone CMake projects for expected failures. Most use a `failing-stage` target that succeeds only when generation or discovery fails with the exact expected diagnostic; `ManifestUpdates` checks the full update, review, rebuild, and execution lifecycle.
- `Bad/RunBadExample.cmake` rejects configuration failures before invoking the declared failing stage.
- Each project under `Bad` owns its feature, manifest, fixture, and CMake target. It calls `zucchinify()` and compiles its own generated test executable; no Bad project borrows a Good executable or manifest.

Each new feature or compiler-boundary fix should add a good example, a bad example, or both, depending on whether successful behavior, diagnostics, or both are part of the contract.

Unit-test references below name their owning source and parameter case; CTest may expose parameterized cases under a generated GoogleTest name.

## Coverage

| Boundary | Coverage |
| --- | --- |
| Malformed manifests | `Bad/MalformedYaml`, `Bad/MissingRequiredField` |
| Invalid C++ identifiers | `Bad/InvalidMethodName`, `Bad/CppKeywordIdentifier` |
| Ambiguous regex matches | `ZucchiniRuntime/Test/MakeZucchiniTest.cpp` (`AmbiguousRegexCaptures`) |
| Missing capture declarations | `ZucchiniRuntime/Test/MakeZucchiniTest.cpp` (`MissingCaptureDeclaration`) |
| Duplicate step definitions | `ZucchiniRuntime/Test/MakeZucchiniTest.cpp` (`DuplicateStepDefinitions`) |
| Invalid and malformed regexes | `Bad/UnanchoredStepRegex`, `ZucchiniRuntime/Test/MakeZucchiniTest.cpp` (`InvalidRegex`) |
| Invalid enum values during discovery | `Bad/InvalidEnumValue` |
| Malformed tables | `Bad/MalformedTable` |
| Missing or misspelled table headers during discovery | `Bad/MissingRequiredTableField`, `Bad/TableHeaderTypo` |
| Repeated table headers, and several alternative headers of one field in a table | `Bad/AmbiguousTableHeaders`, `ZucchiniRuntime/Test/MakeZucchiniTest.cpp` (`RejectsDuplicateHeadersOfDynamicTables`) |
| Malformed or incorrectly typed doc strings during discovery | `Bad/MalformedTypedDocString`, `Bad/WrongTypedDocStringValue` |
| Plain and typed JSON/YAML doc strings, YAML tags, merge keys, and block scalars | `Good/DocStrings` |
| Regex captures that cannot convert to their declared type | `Bad/InvalidRegexArgument`, `ZucchiniRuntime/Test/MakeZucchiniTest.cpp` (`InvalidIntegerCapture`) |
| Undefined-step manifest updates and review diagnostics | `Bad/UndefinedStepSnippet`, `ZucchiniRuntime/Test/FeatureParserTest.cpp` (`Snippets.*`) |
| Optional and defaulted fields, including empty optional strings and custom strings | `Good/Tables`, `Good/LegacyString`, `Good/ScalarLists` |
| Builtin scalar and enum lists in named, column, positional, JSON, and YAML input | `Good/ScalarLists`, `Bad/InvalidScalarList` |
| Optional scalar inference, inferred list element types, and column names containing digits | `Bad/UndefinedStepSnippet` |
| Stylesheets and custom C++ naming/string conventions | `Good/LegacyString`, `Good/NamingConventions` |
| Rules switching over tag-scoped step enums without `default`, built with `-Werror=switch` | `Good/BusinessRules`, `Good/AssertionJustification` |
| Tag generation with verbatim user casing and snake_case generated types | `Good/TagNaming` |
| Unicode | `Good/Notes`, `ZucchiniRuntime/Test/NamingTest.cpp` (`TransliteratesUmlauts`) |
| Windows-style source paths | `ZucchiniRuntime/Test/FeatureParserTest.cpp` (`PreservesWindowsStyleSourcePaths`) |
| Multiple rules and backgrounds | `Good/Calculator` |
| Scenario outlines and name collisions | `Good/Calculator`, `ZucchiniRuntime/Test/FeatureParserTest.cpp` (`KeepsSanitizedTestNamesUnique`) |
| Per-step wrapping with `around_step` | `Good/Notes` |
| Business rules with independent partial models, assertion-based invariant coverage, coverage gaps, and runtime evidence | `Good/BusinessRules` |
| Assertions tied to requirements by business rule coverage, down to table cells named through typed rows; requirement strictness and a report stylesheet | `Good/AssertionJustification`, `ZucchiniRuntime/Test/JustificationTest.cpp` |
| Registered rule failures | `Bad/RuleValidation` |
| Recursive step-tag inheritance, typed membership, and per-tag reverse dispatch | `Good/UserTags`, `Bad/UserTags` |
| Rejection of collisions across final fixture/manifest/tag type names | `ZucchiniGeneratorTest` (`Lowering.RejectsTagStepsNameCollidingWithManifestType`, `Lowering.RejectsGeneratedTypeNamesThatCollideAfterNameIt`) |
| Editing, adding, or removing features relinks and rediscovers without generation or compilation; manifest edits still regenerate | `Good/IncrementalCMake`, exercised by `CMakeIntegration.RebuildsAfterFeatureChanges` |

`Bad.ManifestUpdates` updates the source fixtures directly, checking source-preserving edits, schema rejection, discovery, review failure, idempotence, rebuild, and execution. Checked-in `before.yml.bak` and `after.yml` files show the expected changes; `editing/*/before.yml` are the live editing fixtures. The editing checks run in the generated `ManifestUpdatesTest` executable, linked to the runtime by `zucchinify()`. A parent script restores every manifest from its `.bak` after the assertions finish, including on failure. Before running, it rejects fixture/backup mismatches so local edits are not overwritten.

`Bad/UndefinedStepSnippet` updates its source manifest and checks inferred definitions against `after.yml` and the VS Code review URL against `expected.stderr`, expanding only `@MANIFEST_URL@` for the current machine. It restores the manifest from `before.yml.bak` before reporting assertion failures. A second source fixture, `review manifest #%.yml`, exercises URL escaping.

The storage checks also run inside the generated executable; there is no separate storage helper executable. `Good/ExecutionPlans` exercises the generated executable and runtime storage boundary: sanitized scenario collisions, rediscovery after removals and renames, empty discovery, invalid stored method names, failed directory creation, failed writes, filesystem inspection errors, duplicate plan names, and serialization failure. File-size-limit and symbolic-link failure cases run on Unix. Failed writes preserve existing plan files; unrelated files survive obsolete-plan cleanup.
