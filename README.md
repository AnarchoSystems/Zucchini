# Zucchini

[![CI](https://github.com/AnarchoSystems/Zucchini/actions/workflows/ci.yml/badge.svg)](https://github.com/AnarchoSystems/Zucchini/actions/workflows/ci.yml)

**Zucchini is a Gherkin-to-GoogleTest compiler for C++.**

Write scenarios in [Gherkin](https://cucumber.io/docs/gherkin/), describe their typed C++ boundary in a YAML manifest, and implement the generated GoogleTest fixture interface.

## Getting started

The intended workflow: write the scenario, let the build tell you which step definitions are missing, describe the boundary once, then implement it. In practice:

Say you're building a checkout feature and you write the scenario before the behavior exists:

```gherkin
# features/Checkout.feature
Feature: Checkout

  Scenario: Adding items to a cart
    Given an empty cart
    When I add the following items:
      | name   | price | quantity |
      | Widget | 2.50  | 3        |
    Then the cart total is 7.5
```

Create the required manifest with no definitions yet:

```yaml
# features/Checkout.yaml
steps: []
```

The generated test source includes `Checkout.h`, so provide an initially empty fixture. With no declared steps, it has no step methods to implement yet:

```cpp
// Checkout.h
#pragma once
#include "ICheckout.h"

namespace nCheckout
{
class Checkout : public ICheckout
{
};
}
```

Point `zucchinify()` at the feature directory and build:

```cmake
add_executable(CheckoutTest)

zucchinify(CheckoutTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE Checkout
)
```

Zucchini can now generate the fixture, but discovery cannot bind the feature steps to C++. It fails usefully: for every step with no definition, the runtime prints a ready-to-paste manifest fragment, already typed and grouped into a `types:`/`steps:` shape:

```text
undefined steps in 'features'; add these step definitions:

types:
  - name: IAddTheFollowingItemsRow
    kind: struct
    fields:
      - name: name
      - name: price
        type: float
      - name: quantity
        type: int

steps:
  - step: ^an empty cart$
    methodName: an_empty_cart
  - step: ^I add the following items:$
    methodName: i_add_the_following_items
    dataTable:
      type: IAddTheFollowingItemsRow
  - step: ^the cart total is (-?\d+\.\d+)$
    methodName: the_cart_total_is
    arguments:
      - name: arg1
        type: float
```

Numbers and quoted strings become typed captures: whole numbers as `int`, decimals as `float` (spelled `float` or `double`, both are the same C++ `double`), `"quoted text"` as `string`. Everything else in the step text is matched literally. Each data-table column gets a type the same way, inferred from its own values — falling back to `string` only when nothing narrower fits every row.

Paste this into `features/Checkout.yaml` and rename methods, types, and fields to taste:

```yaml
types:
  - name: Item
    kind: struct
    fields:
      - name: name
        type: string
      - name: price
        type: float
      - name: quantity
        type: int

steps:
  - step: ^an empty cart$
    methodName: reset

  - step: ^I add the following items:$
    methodName: addItems
    dataTable:
      type: Item

  - step: ^the cart total is (-?\d+(?:\.\d+)?)$
    methodName: totalIs
    arguments:
      - name: expected
        type: float
```

From this manifest, Zucchini generates a fixture API in namespace `nCheckout`. The user-facing `ICheckout` is both the typed interface and the GoogleTest parameterized fixture base:

```cpp
namespace nCheckout
{
class CheckoutInterface
{
public:
    virtual ~CheckoutInterface() = default;

    virtual void reset() = 0;
    virtual void addItems(const std::vector<Item>& items) = 0;
    virtual void totalIs(double expected) = 0;
};

class ICheckout
    : public virtual CheckoutInterface
    , public testing::TestWithParam<nZucchini::Zucchini>
{
};
}
```

If the manifest is the API description, this generated interface is the server stub: the thing that turns a contract into code you actually implement. Rebuild, and the snippets are gone; the compiler now reports the step methods that `Checkout` must override. That's the loop: write a scenario, let discovery suggest the missing step definitions, describe the boundary once in YAML, then let the C++ compiler enforce it:

```cpp
// Checkout.h
#pragma once
#include "ICheckout.h"

namespace nCheckout
{
class Checkout : public ICheckout
{
public:
    void reset() override { items.clear(); }

    void addItems(const std::vector<Item>& newItems) override
    {
        items.insert(items.end(), newItems.begin(), newItems.end());
    }

    void totalIs(double expected) override
    {
        double total = 0;
        for (const auto& item : items) total += item.price * item.quantity;
        EXPECT_DOUBLE_EQ(expected, total);
    }

private:
    std::vector<Item> items;
};
}
```

Build and run:

```bash
cmake --build build
ctest --test-dir build
```

No separate Zucchini process remains running. The generated test executable does link `ZucchiniRuntime`, but its parsing work happens when GoogleTest lists/discovers tests: it parses the feature files, matches step regexes, validates and converts arguments, and stores typed scenario plans. When CTest later executes an individual scenario, the runtime only loads its stored plan and calls the generated fixture methods; it does not parse Gherkin or match regexes. `Scenario`s and `Example`s are therefore individual GoogleTest cases, visible in `ctest -N` and IDE test explorers and runnable in parallel with `ctest -j`.

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

Zucchini is a younger, more opinionated project than cucumber-cpp, and describing a manifest up front *is* more ceremony than writing a runtime step-matcher. What it buys you: generated tests that are indistinguishable from hand-written GoogleTest as far as CTest, IDE test explorers, and parallel execution are concerned, plus compile-time and discovery-time checking of the Gherkin/C++ boundary — typos in step arguments, table shapes, and enum values are caught before a single scenario runs, not somewhere inside a runtime matcher.

## How is this different?

**A Cucumber runner that interprets Gherkin during every test run**, such as the [official C++ cucumber implementation](https://github.com/cucumber/cucumber-cpp), parses features and dispatches through runtime-registered step callbacks. Zucchini splits that work across generation, discovery, and execution. The `Zucchini` generator validates the YAML manifest and optional stylesheet, then emits the typed fixture API, conversions, and embedded semantic step definitions. The generated executable links `ZucchiniRuntime`, which parses Gherkin and resolves regexes during GoogleTest discovery, before any scenario test executes.

Discovery turns the resulting [pickles](https://github.com/cucumber/gherkin/tree/main#pickles) into JSON scenario plans containing the selected generated methods and converted arguments. Normal scenario execution loads those plans and calls the generated dispatch code; it does not parse Gherkin or match step regexes again. There is no separate runner process or socket protocol.

**A hand-rolled `TEST_F` with a helper that parses Gherkin yourself** gets you native tests too, but you're back to writing your own argument parsing, table conversion, and source-location bookkeeping for every step.

**GoogleTest's `TEST_P`** covers the scenario-outline use case for parameterized C++ tests, but doesn't give you a Gherkin-readable spec or a typed boundary to a non-C++ audience.

## Scenario-level hooks

Not everything belongs to a single step. Two hooks are intended to give you a more holistic way to interact with your scenarios.

**`validate_scenario`** runs at discovery time, before any step executes. It can reject a scenario for reasons that only make sense at the scenario level, with a diagnostic pointing at the offending feature line — for example, a scenario that asserts on the cart total before any items were ever added, or one that launches the checkout flow twice.

Diagnostics carry a severity of `Error`, `Warning`, or `Info`; only `Error` prevents the scenario from being accepted, so `Warning`/`Info` are useful for flagging smells (an empty `Examples` table, a step that's a no-op) without failing the build.

`validate_scenario` is a great way to manage the complexity of larger projects. Newly onboarded team members or your future self in 18 months will thank you because the alternative is often debugging mysterious runtime errors that have nothing to do with the behavior you are trying to test.

**`around_step`** wraps the execution of an individual step but also lets you see the surrounding context:

```cpp
void around_step(const StepContext& context, const std::function<void()>& step)
{
    // before everything
    ICheckout::around_step(context, [&]() {
      // before the step but after parent class' before-logic
      step();
      // after step but before parent class' after-logic
    });
    // after everything
}
```

Using the `context`, you can for example check if the next step (if there is a next step) asserts an error. You may want to wrap your current step into a try catch block and store the exception for review by the next step and throw only if the next step performs no such assertion.

## CMake integration

```cmake
add_executable(MyTests)

zucchinify(MyTests
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE MyTests
    # Optional:
    # STYLESHEET "${CMAKE_CURRENT_SOURCE_DIR}/features/MyTests.stylesheet.yaml"
)
```

`zucchinify()` expects exactly one YAML manifest in `FEATURE_DIR` (excluding the explicitly supplied `STYLESHEET`). It generates the fixture header and GoogleTest source, adds them to the target, links `Zucchini::Runtime`, and registers scenarios through `gtest_discover_tests`. Generated sources, feature files, the manifest, and the optional stylesheet are CMake dependencies, so changes regenerate and rediscover automatically.


## Command line

The compiler can also be invoked directly, which is useful when inspecting generated code or integrating with a build system other than the supplied CMake module:

```text
Zucchini -i <yaml> -fixture <fixture> [-o <output-directory>] [-style <stylesheet-yaml>]
```

```bash
Zucchini -i features/Checkout.yaml -fixture Checkout -o build/generated
```

## Manifest reference

Beyond scalar arguments and data tables shown above, the manifest supports:

* generated or imported enums and structs (existing C++ types can be referenced instead of generated),
* optional and defaulted table fields, renamed and ignored columns, row- or column-oriented and headerless tables,
* typed doc strings, including structured content (e.g. `"""yaml`) mapped onto a declared C++ type,
* source-location tracking, so diagnostics and generated-scenario failures point back at the originating feature/rule/scenario/step.

The manifest format is defined by [`schemas/schema.json`](schemas/schema.json). The optional code-generation stylesheet is defined by [`schemas/stylesheet.schema.json`](schemas/stylesheet.schema.json). [`Examples/Good`](Examples/Good) contains working end-to-end examples; [`Examples/Bad`](Examples/Bad) contains expected generator and discovery failures.

## Project structure

```text
Zucchini/
├── LibZucchini/      shared definitions and diagnostics
├── ZucchiniRuntime/  generated-test runtime and discovery
├── Zucchini/         command-line compiler and lowering into generated C++
├── cmake/            CMake integration (zucchinify())
├── schemas/          manifest and stylesheet schemas
├── scripts/          repository tooling
├── Examples/
│   ├── Good/          working end-to-end examples
│   └── Bad/           expected generator/discovery failures
└── CMakeLists.txt
```

`LibZucchini` owns the small semantic API shared by generation and discovery: diagnostics and `StepDefinitions`. The `Zucchini` executable owns YAML/schema/stylesheet parsing and code generation, and links only `LibZucchini`. Generated test targets link `ZucchiniRuntime`, which owns Gherkin discovery and scenario-plan loading; the generator does not link the runtime.

## Status

Zucchini is experimental and young — the manifest format and generated interface may still change. It's maintained with the intention of staying that way: the focus is deliberately narrow (compiling Gherkin + a typed manifest into native GoogleTest/CTest), and existing behavior is protected by the `Good`/`Bad` example suite rather than only unit tests.

## Contributing

New compiler behavior should usually come with a `Examples/Good` case (successful generation/execution), an `Examples/Bad` case (an expected diagnostic), or both — the examples are part of the compiler's behavioral contract, not just a demo. See [`Examples/README.md`](Examples/README.md) for how they're wired up.

## Third-party dependencies

Zucchini uses or builds with the following third-party projects:

* [nlohmann/json](https://github.com/nlohmann/json) 3.12.0 — MIT License.
* [json-schema-validator](https://github.com/pboettch/json-schema-validator) 2.3.0 — MIT License.
* [fkYAML](https://github.com/fktn-k/fkYAML) 0.5.0 — MIT License.
* [Gherkin](https://github.com/cucumber/gherkin) 42.0.1 — MIT License. Zucchini uses its C++ parser for feature-file parsing.
* [Cucumber Messages](https://github.com/cucumber/messages) 34.2.0 — MIT License. This is fetched by the Gherkin C++ build.
* [GoogleTest](https://github.com/google/googletest) 1.15.2 — BSD 3-Clause License, used by the tests and generated test targets.
* [tpp](https://github.com/AnarchoSystems/tpp) 0.18.0 — MIT License, used by the code-generation build tools.
* [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) 0.43.1 — MIT License, used by the dependency setup.

These permissive licenses are compatible with Zucchini's MIT-licensed, open-source distribution. When redistributing source or binaries, retain the upstream copyright, license, and disclaimer notices in the source tree or accompanying documentation. If a distribution includes the fetched dependency source trees, retain any additional notices and licenses included in those trees as well.

## License

MIT. See [`LICENSE`](LICENSE).
