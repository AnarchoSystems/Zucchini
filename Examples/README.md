# Example Coverage

The examples are executable proof of Zucchini's generator, discovery, and execution boundaries.

- `Good` contains successful end-to-end scenarios.
- `Bad` contains standalone CMake projects whose `failing-stage` target succeeds only when generation or discovery fails with the exact expected diagnostic.
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
| Malformed or incorrectly typed doc strings during discovery | `Bad/MalformedTypedDocString`, `Bad/WrongTypedDocStringValue` |
| Plain and typed JSON/YAML doc strings | `Good/DocStrings` |
| Regex captures that cannot convert to their declared type | `Bad/InvalidRegexArgument`, `ZucchiniRuntime/Test/MakeZucchiniTest.cpp` (`InvalidIntegerCapture`) |
| Undefined-step suggestions | `Bad/UndefinedStepSnippet`, `ZucchiniRuntime/Test/FeatureParserTest.cpp` (`Snippets.*`) |
| Optional and defaulted fields | `Good/Tables` |
| Stylesheets and custom C++ naming/string conventions | `Good/LegacyString`, `Good/NamingConventions` |
| Unicode | `Good/Notes`, `ZucchiniRuntime/Test/NamingTest.cpp` (`TransliteratesUmlauts`) |
| Windows-style source paths | `ZucchiniRuntime/Test/FeatureParserTest.cpp` (`PreservesWindowsStyleSourcePaths`) |
| Multiple rules and backgrounds | `Good/Calculator` |
| Scenario outlines and name collisions | `Good/Calculator`, `ZucchiniRuntime/Test/FeatureParserTest.cpp` (`KeepsSanitizedTestNamesUnique`) |
| Per-step wrapping with `around_step` | `Good/Notes` |
| `validate_scenario` failures | `Bad/ValidateScenario` |
| Incremental CMake rebuilds | `Good/IncrementalCMake`, exercised by `CMakeIntegration.RebuildsAfterFeatureChanges` |
