# Zucchini

**Zucchini is a Gherkin-to-GoogleTest compiler for C++.**

It lets you write tests in a readable, behavior-oriented format while keeping the actual test implementation in C++.

Zucchini takes a Gherkin feature file together with a YAML step-definition manifest and generates a GoogleTest fixture. The generated fixture connects Gherkin steps, captures, data tables, and doc strings to strongly typed C++ test methods.

The result is a workflow where the test scenario can stay close to the language of the requirements, while the implementation remains ordinary C++ and GoogleTest.

## Why Zucchini?

Traditional C++ tests are excellent for expressing precise technical behavior, but they can become difficult to read when the intent of a test matters more than its implementation details.

Gherkin provides a natural way to describe behavior:

```gherkin
Scenario: Adding two numbers
  Given the calculator has been reset
  When I add 2 and 3
  Then the result should be 5
```

Zucchini bridges that description to C++.

Instead of implementing a separate runtime interpreter for every step, Zucchini generates C++ code from the step definitions. This means that the resulting tests remain native GoogleTest tests and can use the normal C++ toolchain.

## How it works

At a high level, the workflow looks like this:

```text
                 ┌────────────────────┐
                 │   Gherkin .feature │
                 │                    │
                 │ Feature / Scenario │
                 │ Given / When / Then│
                 └─────────┬──────────┘
                           │
                           │ discovery
                           ▼
                 ┌────────────────────┐
                 │      Manifest      │
                 │       YAML         │
                 │                    │
                 │ steps, types,      │
                 │ enums, structs...  │
                 └─────────┬──────────┘
                           │
                           │ Zucchini
                           ▼
                 ┌────────────────────┐
                 │ Generated C++      │
                 │                    │
                 │ fixture header     │
                 │ GoogleTest source  │
                 └─────────┬──────────┘
                           │
                           ▼
                 ┌────────────────────┐
                 │     GoogleTest     │
                 │                    │
                 │  native C++ tests │
                 └────────────────────┘
```

The CMake integration makes the generated sources part of the normal build. Zucchini also hooks the generated tests into GoogleTest discovery, so feature files participate in the regular test workflow.

## Features

Zucchini currently supports:

* Gherkin feature files as human-readable test specifications
* YAML-based step definitions
* Generation of C++ GoogleTest fixtures
* Typed step arguments
* `string`, integer, floating-point, and boolean arguments
* User-defined enumerations
* User-defined structures
* Optional fields
* Default field values
* Data tables
* Typed data tables
* Doc strings
* JSON/YAML and other media-type conversions for typed doc strings
* Imported C++ types
* Custom C++ type names
* CMake integration
* Automatic GoogleTest test discovery
* Whole-scenario hooks such as `validate_scenario` and `aroundStep`

The supported type system and manifest structure are formally described by `schema.json`.

## A small example

A feature can describe behavior without exposing the C++ implementation:

```gherkin
Feature: Calculator

  Scenario: Adding two numbers
    Given the calculator has been reset
    When I add 2 and 3
    Then the result should be 5
```

The YAML manifest describes how those steps map to C++:

```yaml
steps:
  - step: "the calculator has been reset"
    method: reset

  - step: "I add {int} and {int}"
    method: add
    arguments:
      - name: left
        type: int
      - name: right
        type: int

  - step: "the result should be {int}"
    method: result
    arguments:
      - name: expected
        type: int
```

The manifest provides Zucchini with enough information to generate the corresponding C++ fixture and convert captured Gherkin values into the appropriate C++ types.

The important part is that the generated code is ordinary C++ rather than an interpreted test language. Step arguments are converted into appropriate C++ types during generation.

## Using Zucchini from CMake

Zucchini is primarily intended to be integrated through CMake.

A test target can be connected to Zucchini with the `zucchinify()` CMake function:

```cmake
add_executable(CalculatorTest)

zucchinify(CalculatorTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE Calculator
)
```

`zucchinify()`:

1. Finds the YAML manifest in the feature directory.
2. Runs Zucchini to generate the fixture header and test source.
3. Adds the generated files to the target.
4. Links the Zucchini library.
5. Connects the generated tests to GoogleTest discovery.

This keeps code generation inside the normal CMake build rather than requiring a separate manual generation step.

## Building

Clone the repository and configure it with CMake:

```bash
git clone https://github.com/AnarchoSystems/Zucchini.git
cd Zucchini

cmake -S . -B build
cmake --build build
```

To run the tests:

```bash
ctest --test-dir build
```

Because the generated scenarios are native discovered tests, they can also be executed in parallel:

```bash
ctest --test-dir build -j16
```

There is no special parallel execution machinery in Zucchini here — CTest simply sees individual scenarios as tests and can schedule them independently.

## The command-line compiler

Zucchini can also be invoked directly:

```text
Zucchini -i <yaml> -fixture <fixture name> [-o <output dir>]
```

For example:

```bash
Zucchini \
    -i features/steps.yaml \
    -fixture Calculator \
    -o build/generated
```

This generates the C++ files needed for the fixture and its GoogleTest integration.

The command-line interface is intentionally small: provide a YAML step-definition manifest, a fixture name, and optionally an output directory.

## Step definitions and types

The YAML manifest is more than a list of regular expressions. It acts as a type description for the generated C++.

Zucchini understands scalar types such as:

```yaml
type: int
```

```yaml
type: double
```

```yaml
type: bool
```

```yaml
type: string
```

It can also map step arguments to declared enums and structures.

Structures can describe data-table rows, including:

* renamed columns
* optional fields
* default values
* ignored fields
* list fields
* enum-valued fields

This allows a Gherkin table to become a properly typed C++ object instead of forcing the test implementation to manually parse strings.

## Data tables

Data tables can be exposed to C++ as vectors of typed rows.

For example:

```gherkin
When I create the following products:
  | name   | quantity |
  | Widget | 10       |
  | Gear   | 5        |
```

can be represented by a C++ structure and passed to the corresponding step implementation as a typed collection.

Zucchini generates the parsing code required to turn the Gherkin table into those C++ objects. It also supports positional tables when a header is not present.

## Doc strings

Gherkin doc strings can similarly be exposed directly to C++.

For untyped content, a doc string becomes a `std::string`.

For declared structures, Zucchini can generate conversion code so structured content can arrive at the step implementation as the corresponding C++ type. Media types such as JSON and YAML can be used for structured content.

## Whole-scenario interaction

One of the interesting consequences of generating the C++ test rather than merely invoking C++ step functions through a runtime protocol is that Zucchini has a concrete representation of the scenario in the generated code.

That makes it possible to interact with a scenario as a whole.

For example, `validate_scenario` can be used to validate a scenario before execution, while `aroundStep` can wrap step execution and provide cross-cutting behavior around individual steps.

This is a small but important distinction: the generated code isn't just glue between Gherkin and C++. It gives the C++ test implementation a place to reason about the execution of the scenario itself.

## How does Zucchini compare to Cucumber-Cpp?

If you've looked at [Cucumber-Cpp](https://github.com/cucumber/cucumber-cpp), the overall idea will feel familiar: write Gherkin scenarios and implement the steps in C++.

The main difference is how the C++ side is connected to Gherkin.

Cucumber-Cpp uses Cucumber's wire protocol, with Cucumber-Ruby communicating with the C++ step-definition executable. That gives it a nice fit with the wider Cucumber ecosystem, but it also means bringing Ruby into the toolchain and introduces a runtime communication layer.

Zucchini takes a more C++-native approach. It generates the GoogleTest code at build time and lets CMake treat each scenario as a normal test. There is no Ruby dependency and no wire-protocol server involved.

The trade-off is the YAML manifest. Yes, it adds a little ceremony: you explicitly describe how Gherkin steps map to C++ methods and types.

In return, that explicit description gives Zucchini useful information at generation time. The generated C++ knows about the types involved in the scenario, and the codegen itself provides opportunities to interact with the scenario holistically — for example through `validate_scenario` and `aroundStep`.

The GoogleTest integration also makes the resulting tests fit naturally into the existing C++ ecosystem. CTest can execute scenarios in parallel:

```bash
ctest -j16
```

And GoogleTest discovery plays nicely with IDEs such as VS Code: each Gherkin pickle becomes its own filterable test, so individual scenarios can be discovered, run, and debugged directly from the test explorer.

In short, Zucchini trades a little YAML ceremony for a more static, generated, and C++-native testing model.

## Project structure

The repository is split into a few small components:

```text
Zucchini/
├── LibZucchini/     Core parsing, validation and generation logic
├── Zucchini/        Command-line generator
├── Example/         Example project
├── cmake/           CMake integration
├── schema.json      YAML manifest schema
└── CMakeLists.txt
```

`LibZucchini` contains the reusable library functionality, while the `Zucchini` executable provides the command-line interface. The example demonstrates how the CMake integration can be used from a consuming test target.

## Design goals

Zucchini is built around a simple separation of concerns:

**Gherkin describes behavior.**

Feature files should be readable by people and describe what the system under test is expected to do.

**YAML describes the boundary.**

The manifest specifies how Gherkin steps and their data map onto C++ concepts and types.

**C++ implements the behavior.**

The actual test logic remains in C++, where it has access to the complete language, existing libraries, debuggers, IDEs, and the normal GoogleTest ecosystem.

**Code generation connects the two.**

Rather than introducing a runtime interpreter or communication protocol, Zucchini turns the scenario into C++ during the build.

**CMake owns the build.**

Generation is integrated into the build and test process rather than requiring developers to manually run a code-generation step.

This makes Zucchini closer to a compile-time bridge between Gherkin and C++ than to a traditional runtime BDD framework.

## Status

Zucchini is currently an experimental project.

The public API and manifest format may change as the project evolves. It is best suited to experimentation, internal projects, and teams interested in combining Gherkin-style specifications with native C++ tests.

## Contributing

Contributions, experiments, bug reports, and ideas are welcome.

If you find a case where a Gherkin construct does not map cleanly to C++, please consider opening an issue or contributing an example demonstrating the desired behavior.

When contributing, please keep the distinction between the layers in mind:

```text
Gherkin feature
      ↓
YAML manifest
      ↓
generated C++
      ↓
GoogleTest
```

Changes that make this pipeline easier to understand, extend, or debug are particularly valuable.

## License

See the repository's license file for the applicable license terms.

---

**Zucchini** — readable specifications, generated C++, native tests.
