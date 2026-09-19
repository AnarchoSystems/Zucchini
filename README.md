# Zucchini

**Zucchini is a Gherkin-to-GoogleTest compiler for C++.**

It lets you write test scenarios in Gherkin and implement their behavior in ordinary C++. At build time, Zucchini turns the feature files and a YAML step-definition manifest into C++ test code that integrates with GoogleTest.

The generated tests are native C++ tests. There is no runtime Gherkin interpreter or wire protocol involved.

## What it does

A Zucchini test consists of three pieces:

```text
        Gherkin                 YAML
     feature file          step-definition manifest
          │                       │
          └──────────┬────────────┘
                     │
                  Zucchini
                     │
                     ▼
              generated C++
                     │
                     ▼
                GoogleTest
```

Gherkin files describe the scenarios.

The YAML manifest describes how Gherkin steps and their arguments map to C++ methods and types.

Zucchini generates the C++ that connects the two.

CMake integration makes the generated sources part of the normal build, and GoogleTest discovery makes the generated scenarios available through the usual test tooling.

## Example

A feature file can describe the behavior:

```gherkin
Feature: Calculator

  Scenario: Adding two numbers
    Given the calculator has been reset
    When I add 2 and 3
    Then the result should be 5
```

The corresponding step definitions can be described in YAML:

```yaml
steps:
  - step: "^the calculator has been reset$"
    methodName: reset

  - step: "^I add ([0-9]+) and ([0-9]+)$"
    methodName: add
    arguments:
      - name: left
        type: int
      - name: right
        type: int

  - step: "^the result should be ([0-9]+)$"
    methodName: result
    arguments:
      - name: expected
        type: int
```

The `step` entries are regular expressions. Captured values are converted to the types declared in `arguments`.

The generated test ultimately calls ordinary C++ methods with typed arguments.

## Gherkin support

Zucchini currently handles the following Gherkin structures:

* `Feature`
* `Background`
* `Rule`
* `Scenario`
* `Scenario Outline`
* `Examples`
* `Given`, `When`, `Then`, `And`, and `But`
* equivalent constructs in various languages as defined by the [Gherkin parser](https://github.com/cucumber/gherkin/blob/main/gherkin-languages.json)
* data tables
* doc strings

Scenario outlines are expanded into individual generated tests using their `Examples` rows.

Generated tests retain the source information needed to relate them back to the original feature, rule, scenario, and step locations.

For example:

```gherkin
Feature: Calculator

  Rule: Addition

    Scenario Outline: Adding numbers
      When I add <left> and <right>
      Then the result should be <total>

      Examples:
        | left | right | total |
        | 1    | 2     | 3     |
        | 10   | -3    | 7     |
```

becomes separate generated test cases for the example rows.

## Typed step arguments

The YAML manifest acts as a type description for the boundary between Gherkin and C++.

Scalar arguments can be mapped to C++ types such as:

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

The manifest can also describe user-defined structures and enumerations.

This means that test implementations don't need to receive everything as strings and perform their own parsing. Zucchini generates the conversions based on the declared types.

## Data tables

Gherkin data tables can be mapped to typed C++ structures.

For example:

```gherkin
When I create the following entries:
  | name   | quantity |
  | Widget | 10       |
  | Gear   | 5        |
```

A manifest can describe the structure expected by the step, allowing the generated code to construct typed values from the table.

Tables can be interpreted by rows or by columns, and the manifest can describe properties such as:

* renamed columns
* optional fields
* default values
* ignored fields
* typed fields
* enum-valued fields

Tables without a header can also be represented positionally.

## Doc strings

Doc strings can be passed to C++ as typed values.

Plain doc strings can be consumed as strings, while the manifest can declare a structured C++ type.

Media types are supported for typed doc strings. Zucchini can therefore select the appropriate conversion for structured content such as JSON or YAML.

For example, a feature can contain:

```gherkin
Given the configuration:
  """yaml
  enabled: true
  retries: 3
  """
```

and the manifest can associate that content with a C++ type.

## C++ types

Zucchini can generate definitions for types described by the manifest, but it can also work with types that already exist in the C++ project.

Imported enums and structures can be referenced without generating another definition.

The manifest also supports explicit C++ type names where the generated type name needs to differ from the manifest name.

The manifest format is described by the repository's `schema.json`.

## Scenario-level functionality

Zucchini generates code for the scenario as a whole, not only individual step calls.

Two pieces of scenario-level functionality are currently exposed:

`validate_scenario` allows scenario validation at discovery time. Add diagnostics to report nonsensical scenarios (eg setup done after application launch, forgotten setup without meaningful fallback,...) without even running the test. The hook returns `void`; an error-severity diagnostic stops discovery.

Diagnostics have a severity of `Error`, `Warning`, or `Info`. All diagnostics are printed, but discovery only fails when at least one diagnostic has error severity.

`around_step` allows code to wrap the execution of individual steps. For example, you could wrap your step in a try-catch block and store the error if the next step happens to be an assertion on the error and throw it otherwise. Or on error, you could set a flag to start writing debug output to a file and rerun the step that threw an error.

These hooks give generated tests a place for behavior that concerns the execution of the scenario itself rather than one particular step.

## CMake integration

Zucchini is designed to be used from CMake.

The CMake integration connects a test target to a feature directory and fixture:

```cmake
add_executable(CalculatorTest)

zucchinify(CalculatorTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE Calculator
)
```

The integration takes care of generating the C++ sources and adding them to the consuming target.

The resulting tests are integrated with GoogleTest discovery, so the generated scenarios can participate in the normal CMake/CTest test workflow.

All you have to do is to write the class you specified as FIXTURE in a .h file with that same name while inheriting from I\<your fixture\>, for example ICalculator.

## Command-line usage

The generator can also be invoked directly:

```text
Zucchini -i <yaml> -fixture <fixture name> [-o <output dir>]
```

For example:

```bash
Zucchini \
    -i features/Calculator.yaml \
    -fixture Calculator \
    -o build/generated
```

The command generates the C++ sources for the fixture.

## Building

Zucchini is built with CMake:

```bash
git clone https://github.com/AnarchoSystems/Zucchini.git
cd Zucchini

cmake -S . -B build
cmake --build build
```

Run your test suite with:

```bash
ctest --test-dir build
```

CTest can also run the generated tests in parallel:

```bash
ctest --test-dir build -j16
```

There is no separate Zucchini test runner involved here. The generated scenarios are GoogleTest tests and are therefore handled by the normal CTest/GoogleTest tooling.

## Project structure

The repository is organized into a few main components:

```text
Zucchini/
├── LibZucchini/     Core parsing, validation and code generation
├── Zucchini/        Command-line generator
├── Example/         Example project and feature files
├── cmake/           CMake integration
├── schema.json      Step-definition manifest schema
└── CMakeLists.txt
```

`LibZucchini` contains the reusable compiler functionality.

`Zucchini` contains the command-line executable.

`Example` contains a working example of the feature files, manifests, generated tests, and CMake integration.

## Design

The central idea is to keep the four concerns separate:

```text
Gherkin
  │
  │ describes behavior
  ▼
YAML manifest
  │
  │ describes the C++ boundary
  ▼
generated C++
  │
  │ provides a test interface class with
  │ typed pure virtual step methods
  ▼
your C++ step implementation
  │
  │ defines the actual behavior for each step definition
  ▼
GoogleTest
```

Gherkin provides the human-readable specification.

The manifest provides the information that cannot be inferred from the Gherkin text alone: which C++ method a step calls, what its arguments are, and what types should be generated.

The generated C++ provides you with an abstract class to implement and knows how to map Gherkin steps to method calls with typed arguments on your class.

You may think of the manifest as a protocol, similar to OpenAPI specs. In this analogy, the generated abstract class would be the interface of your service.

The final result is a normal GoogleTest-based C++ test rather than a test interpreted by a separate runtime.

## Current scope

Zucchini is intentionally centered around generating native C++ tests from Gherkin.

It currently provides:

* Gherkin parsing and scenario compilation
* `Background`, `Rule`, and scenario support
* scenario outlines and examples
* regex-based step definitions
* typed step arguments
* typed data tables
* typed doc strings
* JSON/YAML and other media-type conversions for typed doc strings
* generated and imported C++ structures and enums
* optional and defaulted structure fields
* renamed and ignored table fields
* source-location information for generated scenarios
* scenario validation
* step-wrapping with `around_step`
* CMake integration
* GoogleTest integration and test discovery

The YAML manifest format is formally described by `schema.json`.

## Status

Zucchini is an experimental project and its API and manifest format may change.

The repository currently focuses on the compiler, generated C++ test code, and its CMake/GoogleTest integration.

## License

See the repository's license file for the applicable license terms.
