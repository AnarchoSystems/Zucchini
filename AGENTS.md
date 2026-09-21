# Agent Notes

Use parallelism for project validation:

```sh
cmake --build build -j"$(nproc)"
ctest --test-dir build --parallel "$(nproc)" --output-on-failure
```
