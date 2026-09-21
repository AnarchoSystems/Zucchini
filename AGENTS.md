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
