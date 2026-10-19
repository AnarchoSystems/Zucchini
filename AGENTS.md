# Agent Notes

## Target layout and API ownership

- Every build target has its own top-level directory. Tests for a target live in that target's `Test` subdirectory. The `Examples` tree is exempt.
- Every library target has its own public include directory and an umbrella header in that directory. Shared headers use `include/Zucchini`; runtime headers use `include/Zucchini/Runtime`.
- Keep `LibZucchini` minimal: it contains only API and implementation genuinely shared by the `Zucchini` generator and `ZucchiniRuntime`.
- `ZucchiniRuntime` contains everything generated sources need but the generator does not. The `Zucchini` executable must not link it.
- Generator-only concerns, including stylesheet and manifest parsing, belong directly to the `Zucchini` target rather than a support library.
- Prefer one class per header. Group classes only when they are small parts of one semantic unit.
- Group free functions by meaning. Functions used only within one compilation unit belong in that compilation unit's anonymous namespace.

## Writing rules

Rules must stay independent of the step vocabulary. This applies to examples,
documentation snippets, tests, and any rule you write or propose for users.
The structure below is a convention Zucchini documents, not one it enforces:

- A manifest tag names one fact a step states (`Transaction`, `Observation`), not
  the rules that read it. Tag a step with what it says.
- Each tag has exactly one read-only extractor,
  `<Tag>Fact extractFact(<Tag>Steps, const ZucchiniStep &)`, built from the
  generated `<method>Args(step)` types. It is the only code that switches over
  `<Tag>Steps`, and it uses no scenario state.
- Rules consume facts, never `StepMethod` or `<Tag>Steps`; see `onFact` and
  the rules in `Examples/Good/BusinessRules`. Each fact alternative gets an
  overload; ignored alternatives get an explicit empty overload.
- Never write `default:` in a switch over `StepMethod` or `<Tag>Steps`.
  `Examples/Good` builds with `-Werror=switch`.
- Make a fact a `std::variant` where the concept may grow, so a new alternative
  forces every consuming rule to handle it.
- Switch over the full `StepMethod` (still without a default) only in rules that
  must place every step, such as protocol/ordering rules.
- A small fixture may switch over one tag directly in its rule
  (`Examples/Good/AssertionJustification`); prefer the fact layer otherwise.
- When adding a step, tag it with the facts it states and extend those
  extractors. Do not edit rules unless a fact type changed.

See `Doc/Rules.md#decouple-rules-from-the-step-vocabulary`.

## Validation

Use parallelism for project validation:

```sh
cmake --build build -j"$(nproc)"
ctest --test-dir build --parallel "$(nproc)" --output-on-failure
```

## Windows portability

Windows is a supported CI platform. Consider Windows behavior when changing code,
CMake integration, or tests; passing locally on Linux is not enough to establish
that a change works on Windows.

- Account for drive letters, backslashes, paths containing spaces, and CRLF line
  endings. Quote paths and use CMake's portable commands where possible.
- Support multi-configuration generators such as Visual Studio: pass the selected
  configuration when building or invoking CTest, and resolve executable paths
  through target generator expressions.
- CMake can wrap diagnostic text across lines, particularly with long Windows
  paths. Tests inspecting diagnostics through CMake/CTest should tolerate that
  wrapping when whitespace is not part of the behavior being tested. For example,
  `Examples/Bad/PrintStepSnippet/AssertPrint.cmake` normalizes whitespace before
  checking snippet output. Preserve exact comparisons where formatting matters.
- Check relevant Windows CI results when available, and state when validation was
  limited to another platform.
