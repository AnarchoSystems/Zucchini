# Agent Notes

## Target layout and API ownership

- Every build target has its own top-level directory. Tests for a target live in that target's `Test` subdirectory. The `Examples` tree is exempt.
- Every library target has its own public include directory and an umbrella header in that directory. Shared headers use `include/Zucchini`; runtime headers use `include/Zucchini/Runtime`.
- Keep `LibZucchini` minimal: it contains only API and implementation genuinely shared by the `Zucchini` generator and `ZucchiniRuntime`.
- `ZucchiniRuntime` contains everything generated sources need but the generator does not. The `Zucchini` executable must not link it.
- Generator-only concerns, including stylesheet and manifest parsing, belong directly to the `Zucchini` target rather than a support library.
- Prefer one class per header. Group classes only when they are small parts of one semantic unit.
- Group free functions by meaning. Functions used only within one compilation unit belong in that compilation unit's anonymous namespace.

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
