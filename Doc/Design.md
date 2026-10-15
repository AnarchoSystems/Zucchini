# Design and project notes

[Overview](../README.md) · [Working with Zucchini](WorkingWithZucchini.md)

## When to use this — and when not to

### BDD in general

Zucchini is just one implementation of [Behavior Driven Development (BDD)](https://cucumber.io/docs/bdd/), so it is worth saying a few words about this.

BDD-style scenarios aren't a replacement for unit tests. In fact, they are not even primarily a testing tool at all — they are a way to keep a written expectation of behavior in sync with what the software actually does.

Gherkin files are specifications, their main purpose is to communicate with non-technical stakeholders. You take unstructured specs and turn them into specs that just *happen* to be structured in such a way that you can infer tests from them - and doing so while employing TDD principles keeps these specs in sync with reality.

Reach for BDD when:

* the behavior is worth describing in a language a non-C++ reader (product, QA, a future maintainer) can review,
* you want that description to fail the build the moment it drifts from the implementation, rather than living in a wiki page.

Don't reach for it to unit-test a single function, or for anything where the ceremony of feature files outweighs the benefit of a readable spec — a plain `TEST_F` is simpler and that's fine.

### Zucchini in particular

If you are already sold on BDD and your code base is C++, here's when to use Zucchini. Use Zucchini when:

* you want native CTest integration with individually discoverable, parallel-friendly tests,
* you want a generated, typed C++ boundary instead of runtime callback registration,
* you do not want a separate runner process or service,
* you want invalid manifests, arguments, tables, and doc strings to fail before scenario execution.

Zucchini isn't trying to be any of these:

* no standalone test runner — GoogleTest/CTest already are one,
* no assertion library of its own — you use GoogleTest's,
* no scripting language, and no remote test-execution protocol.

Zucchini takes a more opinionated approach than cucumber-cpp, and describing a manifest up front *is* more ceremony than writing a runtime step-matcher. What it buys you: generated tests that are indistinguishable from hand-written GoogleTest as far as CTest, IDE test explorers, and parallel execution are concerned, plus compile-time and discovery-time checking of the Gherkin/C++ boundary — typos in step arguments, table shapes, and enum values are caught before a single scenario runs, not somewhere inside a runtime matcher.

## How is this different?

**A Cucumber runner that interprets Gherkin during every test run**, such as the [official C++ cucumber implementation](https://github.com/cucumber/cucumber-cpp), parses features and dispatches through runtime-registered step callbacks. Zucchini splits that work across generation, discovery, and execution. The `Zucchini` generator validates the YAML manifest and optional stylesheet, then emits the typed fixture API, conversions, and embedded semantic step definitions. During GTest's list-tests pass, the generated test `.cc` asks the GTest-free `ZucchiniRuntime` to parse Gherkin, build execution plans, and register one test per plan through GTest's public `RegisterTest` API.

Discovery turns the resulting [pickles](https://github.com/cucumber/gherkin/tree/main#pickles) into JSON scenario plans containing the selected generated methods and converted arguments. Normal scenario execution loads those plans and calls the generated dispatch code; it does not parse Gherkin or match step regexes again. There is no separate runner process or socket protocol.

**A hand-rolled `TEST_F` with a helper that parses Gherkin yourself** gets you native tests too, but you're back to writing your own argument parsing, table conversion, and source-location bookkeeping for every step.

**GoogleTest's `TEST_P`** covers the scenario-outline use case for parameterized C++ tests, but doesn't give you a Gherkin-readable spec or a typed boundary to a non-C++ audience.

## Project structure

```text
Zucchini/
├── LibZucchini/      shared definitions, manifest parsing and validation
├── ZucchiniRuntime/  generated-test runtime, discovery and manifest editing
├── Zucchini/         command-line compiler and lowering into generated C++
├── ZucchiniVSCode/   rules explorer, execution, and AI scenario drafts
├── Doc/              installation, workflow, rules, and design guides
├── cmake/            CMake integration (zucchinify())
├── schemas/          manifest, stylesheet, and rule report schemas
├── scripts/          repository tooling
├── Examples/
│   ├── Good/          working end-to-end examples
│   └── Bad/           expected generator/discovery failures
└── CMakeLists.txt
```

`LibZucchini` owns the definitions, diagnostics, YAML manifest parsing, and schema validation shared by the generator and runtime. The `Zucchini` executable owns stylesheet parsing, lowering, and code generation. Generated test targets link `ZucchiniRuntime`, which owns Gherkin discovery, source-preserving manifest updates, and scenario-plan storage and execution. The generator does not link the runtime; manifest updates run directly in the generated test executable.

## Status

I’m using Zucchini for more and more tests in private repositories, so major breaking API changes are becoming less likely. This isn’t a stable API guarantee, but I expect upcoming work to focus on features and bug fixes rather than fundamental changes.
The project connects Gherkin specifications and typed C++ steps to native GoogleTest/CTest, with rules and reports that show which requirements those scenarios check.

## Contributing

New compiler behavior should usually come with a `Examples/Good` case (successful generation/execution), an `Examples/Bad` case (an expected diagnostic), or both — the examples are part of the compiler's behavioral contract, not just a demo. See [`Examples/README.md`](../Examples/README.md) for how they're wired up.

## Third-party dependencies

Zucchini uses or builds with the following third-party projects:

* [nlohmann/json](https://github.com/nlohmann/json) 3.12.0 — MIT License.
* [json-schema-validator](https://github.com/pboettch/json-schema-validator) 2.3.0 — MIT License.
* [libfyaml](https://github.com/pantoniou/libfyaml) 0.9.6 — MIT License.
* [Gherkin](https://github.com/cucumber/gherkin) 42.0.1 — MIT License. Zucchini uses its C++ parser for feature-file parsing.
* [Cucumber Messages](https://github.com/cucumber/messages) 34.2.0 — MIT License. This is fetched by the Gherkin C++ build.
* [GoogleTest](https://github.com/google/googletest) 1.12.1 or newer — BSD 3-Clause License, used by the tests and generated test targets. Google Mock is not built.
* [tpp](https://github.com/AnarchoSystems/tpp) 0.18.0 — MIT License, used by the code-generation build tools.
* [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) 0.43.1 — MIT License, used by the dependency setup.

These permissive licenses are compatible with Zucchini's MIT-licensed, open-source distribution. When redistributing source or binaries, retain the upstream copyright, license, and disclaimer notices in the source tree or accompanying documentation. If a distribution includes the fetched dependency source trees, retain any additional notices and licenses included in those trees as well.
