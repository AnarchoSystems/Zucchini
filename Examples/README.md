# Example coverage

The examples are executable proof of Zucchini's compiler boundaries.

- `Good` contains successful end-to-end scenarios.
- `Bad` contains standalone CMake projects whose `failing-stage` target succeeds only when the declared stage fails with the exact expected diagnostic or normalized test output.
- `Bad/RunBadExample.cmake` rejects configuration failures before invoking the declared failing stage.
- Each project under `Bad` owns its feature, manifest, fixture, and CMake target. It calls `zucchinify()` and compiles its own generated test executable; no Bad project borrows a Good executable or manifest.

Each new feature or compiler-boundary fix should add a good example, a bad example, or both, depending on whether successful behavior, diagnostics, or both are part of the contract.

## Adversarial coverage

| Boundary | Coverage |
| --- | --- |
| Malformed manifests | `Bad/MalformedYaml`, `Bad/MissingRequiredField` |
| Ambiguous regex matches | `Pickles/LinkingFailures.../AmbiguousRegexCaptures` |
| Missing capture declarations | `Pickles/LinkingFailures.../MissingCaptureDeclaration` |
| Duplicate step definitions | `Pickles/LinkingFailures.../DuplicateStepDefinitions` |
| Invalid and malformed regexes | `Bad/UnanchoredStepRegex`, `Pickles/LinkingFailures.../InvalidRegex` |
| Invalid enum values during discovery | `Bad/InvalidEnumValue` |
| Malformed tables | `Bad/MalformedTable` |
| Missing or misspelled table headers during discovery | `Bad/MissingRequiredTableField`, `Bad/TableHeaderTypo` |
| Malformed or incorrectly typed doc strings during discovery | `Bad/MalformedTypedDocString`, `Bad/WrongTypedDocStringValue` |
| Regex captures that cannot convert to their declared type | `Bad/InvalidRegexArgument`, `Pickles/LinkingFailures.../InvalidIntegerCapture` |
| Optional and defaulted fields | `Good/Tables` |
| Unicode | `Good/Notes`, `Names/Naming.../TransliteratesUmlauts` |
| Windows-style source paths | `FeatureParser.PreservesWindowsStyleSourcePaths` |
| Multiple rules and backgrounds | `Good/Calculator` |
| Scenario outlines and name collisions | `Good/Calculator`, `FeatureParser.KeepsSanitizedTestNamesUnique` |
| C++ keyword identifiers | `Bad/CppKeywordIdentifier` |
| Duplicate generated test names | `FeatureParser.KeepsSanitizedTestNamesUnique` |
| `validate_scenario` failures | `Bad/ValidateScenario` |
| Ordinary test assertion failures | `Bad/TestAssertion` |
| Incremental CMake rebuilds | `Good/IncrementalCMake`, exercised by `CMakeIntegration.RebuildsAfterFeatureChanges` |
