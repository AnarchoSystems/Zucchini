# Zucchini

[![CI](https://github.com/AnarchoSystems/Zucchini/actions/workflows/ci.yml/badge.svg)](https://github.com/AnarchoSystems/Zucchini/actions/workflows/ci.yml)

**Zucchini is a Gherkin-to-GoogleTest compiler for C++.**

Write scenarios in [Gherkin](https://cucumber.io/docs/gherkin/), implement the behavior in ordinary C++, glue them together using a yaml manifest.

## Getting started

The intended workflow: write the scenario, let the build tell you which step definitions are missing, describe the boundary once, then implement it. In practice:

Say you're building a checkout feature and you write the scenario before the code exists:

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

Point `zucchinify()` at the feature directory and build:

```cmake
add_executable(CheckoutTest)

zucchinify(CheckoutTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE Checkout
)
```

Before Zucchini can generate anything, it needs an OpenAPI-style contract binding the Gherkin vocabulary to C++: which method each step calls, what its arguments are, and what their types are. That contract is the manifest, and you haven't written one yet — so the build fails during test discovery. But it fails usefully: for every step with no definition, Zucchini prints a ready-to-paste fragment of that contract, already typed and grouped into a `types:`/`steps:` shape:

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

From this manifest, Zucchini generates a pure-virtual interface:

```cpp
class ICheckout
{
public:
    virtual ~ICheckout() = default;

    virtual void reset() = 0;
    virtual void addItems(const std::vector<Item>& items) = 0;
    virtual void totalIs(double expected) = 0;
};
```

If the manifest is the API description, this generated interface is the server stub: the thing that turns a contract into code you actually implement. Rebuild, and the snippets are gone — replaced by a single, unambiguous compiler error: `Checkout` doesn't implement `ICheckout` yet. That's the loop: write a scenario, let the build tell you the step definitions it's missing, describe the boundary once in YAML, then implement:

```cpp
// Checkout.h
#pragma once
#include "ICheckout.h"

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
```

Build and run:

```bash
cmake --build build
ctest --test-dir build
```

No separate Zucchini runner is involved. `Scenario`s and `Example`s are each discovered as individual GoogleTest cases at build/discovery time, so they show up by name in `ctest -N`, in your IDE's test explorer, and can run in parallel with `ctest -j`.

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

* you want good ctest integration that allows discovering tests individually and is parallelism-friendly
* you don't want to depend on an external runtime
* you care about failing fast

Zucchini isn't trying to be any of these:

* no runtime Gherkin interpreter,
* no standalone test runner — GoogleTest/CTest already are one,
* no assertion library of its own — you use GoogleTest's,
* no scripting language, and no remote test-execution protocol.

Zucchini is a younger, more opinionated project than cucumber-cpp, and describing a manifest up front *is* more ceremony than writing a runtime step-matcher. What it buys you: generated tests that are indistinguishable from hand-written GoogleTest as far as CTest, IDE test explorers, and parallel execution are concerned, plus compile-time and discovery-time checking of the Gherkin/C++ boundary — typos in step arguments, table shapes, and enum values are caught before a single scenario runs, not somewhere inside a runtime matcher.

## How is this different?

**A Cucumber runner that interprets Gherkin at runtime**, such as the [official C++ cucumber implementation](https://github.com/cucumber/cucumber-cpp), parses feature files and dispatches to step definitions while the test is running. Zucchini instead compiles the manifest into ordinary C++ once, at build time: the interface, the argument conversions, the table/doc-string plumbing. At test execution time, there is no regex matching to look up steps, there is no Gherkin parsing and there is no external runner communicating with your test executable via a socket. Everything happens in C++ and ctest/gtest.

The only Gherkin parsing that *is* happening happens at test-discovery time. The discovered scenarios (or technically speaking ["pickles"](https://github.com/cucumber/gherkin/tree/main#pickles)) are processed into a typed execution plan — which generated step methods to call, in what order, with which already-converted arguments — that the generated GoogleTest case just replays. cucumber-cpp is to Zucchini roughly as a runtime RPC framework is to an OpenAPI-generated client/server pair: same idea, but the code doing the talking is generated instead of interpreted.

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
)
```

`zucchinify()` expects exactly one YAML manifest in `FEATURE_DIR`, generates the fixture interface and GoogleTest source from it and the feature files, adds them to the target, and registers the scenarios with `gtest_discover_tests` so they participate in the normal CTest workflow. The generated sources, feature files, and manifest are all wired in as CMake dependencies, so a build regenerates and rediscovers automatically whenever any of them change.


## Command line

The compiler can also be invoked directly, which is useful when inspecting generated code or integrating with a build system other than the supplied CMake module:

```text
Zucchini -i <yaml> -fixture <fixture> [-o <output-directory>]
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

The full format is defined by [`schemas/schema.json`](schemas/schema.json). [`Examples/Good`](Examples/Good) has worked examples for each of these; [`Examples/Bad`](Examples/Bad) has the corresponding compiler diagnostics.

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
│   ├── Good/        working end-to-end examples
│   └── Bad/         expected compiler failures
└── CMakeLists.txt
```

## Status

Zucchini is experimental and young — the manifest format and generated interface may still change. It's maintained with the intention of staying that way: the focus is deliberately narrow (compiling Gherkin + a typed manifest into native GoogleTest/CTest), and existing behavior is protected by the `Good`/`Bad` example suite rather than only unit tests.

## Contributing

New compiler behavior should usually come with a `Examples/Good` case (successful generation/execution), an `Examples/Bad` case (an expected diagnostic), or both — the examples are part of the compiler's behavioral contract, not just a demo. See [`Examples/README.md`](Examples/README.md) for how they're wired up.

## License

MIT. See [`LICENSE`](LICENSE).
