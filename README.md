# Zucchini

[![CI](https://github.com/AnarchoSystems/Zucchini/actions/workflows/ci.yml/badge.svg)](https://github.com/AnarchoSystems/Zucchini/actions/workflows/ci.yml)

**Zucchini is a Gherkin-to-GoogleTest compiler for C++.**

Write scenarios in [Gherkin](https://cucumber.io/docs/gherkin/), describe their typed C++ boundary in a YAML manifest, and implement the generated GoogleTest fixture interface.

## Getting started

The intended workflow: write the scenario, let discovery add missing step definitions to the manifest, review the typed boundary, then implement it. In practice:

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

Discovery finds the missing definitions and inserts them into `features/Checkout.yaml`. It then fails with a clickable review diagnostic (the absolute path depends on your checkout):

```text
undefined step definitions. please review vscode://file/path/to/project/features/Checkout.yaml:1:1
```

The updated manifest contains:

```yaml
steps:
  - step: '^an empty cart$'
    methodName: an_empty_cart

  - step: '^I add the following items:$'
    methodName: i_add_the_following_items
    dataTable:
      type: IAddTheFollowingItemsRow

  - step: '^the cart total is (-?\d+\.\d+)$'
    methodName: the_cart_total_is
    arguments:
      - name: arg1
        type: float

types:
  - name: IAddTheFollowingItemsRow
    kind: struct
    fields:
      - name: name

      - name: price
        type: float

      - name: quantity
        type: int
```

Numbers and quoted strings in step text become typed captures: whole numbers as `int`, decimals as `float` (spelled `float` or `double`, both are the same C++ `double`), `"quoted text"` as `string`. Everything else in the step text is matched literally.

Data-table columns infer `int`, `float`, `bool`, or `string` from their nonempty values. Empty cells make the field optional rather than forcing it to `string`; an empty optional string cell is absent. Commas suggest a `list`, with its element type inferred across the values (for example, `content: int` for integers). Mixed values fall back to `string` when no narrower type fits. Column names retain digits: `Column 1` and `Column 2` become `column1` and `column2` under the default casing.

Examples that produce the same suggested regex are combined into one step
definition, even when their captured strings or numbers differ. Table columns
and inferred types are merged across those examples, including across feature
files, and suggestions retain first-seen order.

Review the updated `features/Checkout.yaml` and rename methods, types, and fields to taste:

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

From this manifest, Zucchini generates a fixture API in namespace `nCheckout`. The user-facing `ICheckout` is GTest-free; its interface inherits the runtime scenario hooks. The generated `.cc` supplies a GoogleTest fixture derived from your `Checkout` class:

```cpp
namespace nCheckout
{
class ICheckoutInterface
    : public nZucchini::ScenarioFixture<StepMethod, detail::resolve_step_method>
{
public:
    virtual ~ICheckoutInterface() = default;

    virtual void reset() = 0;
    virtual void addItems(const std::vector<Item>& items) = 0;
    virtual void totalIs(double expected) = 0;
};

class ICheckout
    : public virtual ICheckoutInterface
{
};
}
```

If the manifest is the API description, this generated interface is the server stub: the thing that turns a contract into code you actually implement. Rebuild after reviewing the manifest; the compiler now reports the step methods that `Checkout` must override. That's the loop: write a scenario, let discovery add the missing step definitions, review the boundary in YAML, then let the C++ compiler enforce it:

```cpp
// Checkout.h
#pragma once
#include "ICheckout.h"
#include <gtest/gtest.h>

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

Zucchini takes a more opinionated approach than cucumber-cpp, and describing a manifest up front *is* more ceremony than writing a runtime step-matcher. What it buys you: generated tests that are indistinguishable from hand-written GoogleTest as far as CTest, IDE test explorers, and parallel execution are concerned, plus compile-time and discovery-time checking of the Gherkin/C++ boundary — typos in step arguments, table shapes, and enum values are caught before a single scenario runs, not somewhere inside a runtime matcher.

## How is this different?

**A Cucumber runner that interprets Gherkin during every test run**, such as the [official C++ cucumber implementation](https://github.com/cucumber/cucumber-cpp), parses features and dispatches through runtime-registered step callbacks. Zucchini splits that work across generation, discovery, and execution. The `Zucchini` generator validates the YAML manifest and optional stylesheet, then emits the typed fixture API, conversions, and embedded semantic step definitions. During GTest's list-tests pass, the generated test `.cc` asks the GTest-free `ZucchiniRuntime` to parse Gherkin, build execution plans, and register one test per plan through GTest's public `RegisterTest` API.

Discovery turns the resulting [pickles](https://github.com/cucumber/gherkin/tree/main#pickles) into JSON scenario plans containing the selected generated methods and converted arguments. Normal scenario execution loads those plans and calls the generated dispatch code; it does not parse Gherkin or match step regexes again. There is no separate runner process or socket protocol.

**A hand-rolled `TEST_F` with a helper that parses Gherkin yourself** gets you native tests too, but you're back to writing your own argument parsing, table conversion, and source-location bookkeeping for every step.

**GoogleTest's `TEST_P`** covers the scenario-outline use case for parameterized C++ tests, but doesn't give you a Gherkin-readable spec or a typed boundary to a non-C++ audience.

## Scenario contexts and hooks

Not everything belongs to a single step. [Rules](#rules-and-requirements) check
that scenarios make sense before they run; `around_step` lets you add shared
behavior around each step during execution. Both can inspect the scenario and
its typed step arguments.

Each argument-taking method has a generated Args aggregate. By default its type is `<operation id>Args`; the type naming rule's `onArgsType` block can customize it. `StepView::getArgs<StepMethod::operation>()` and `ScenarioContext::getArgs<StepMethod::operation>(step)` return that concrete type. Methods without arguments have no Args type, so attempting to request one is a compile-time error. A method/step mismatch at runtime throws `std::logic_error`.

Manifest steps may declare identifier tags with `tags: [IntroducesUser]`; recursive `group: { tags, steps }` entries apply their tags to every descendant step. Effective tags are the deduplicated union of ancestor and step tags. `ScenarioContext::tags(step)` returns the generated fixture's `TagSet` alias, so hooks can query membership with `contains(StepTag::IntroducesUser)`. The generated `StepTag` values are dense, zero-based bit indices; `TagSet::contains` returns false for out-of-range values. These manifest step tags are distinct from Gherkin scenario tags.

For reverse lookup, each tag generates a scoped `<Tag>Steps` enum. `context.cast<StepTag::IntroducesUser>(context.method(step))` returns that enum or throws when the method does not carry the tag; the namespace-level `cast<StepTag::IntroducesUser>(method)` helper is also available. Generated dispatch covers every `StepMethod` without a `default`. User-written switches over `<Tag>Steps` can receive compiler warnings when new tagged methods are added; making those warnings build failures requires warning-as-error compiler options.

Type-name casing applies to generated identifiers, not user-provided names. Fixture names from CMake and names from the manifest keep their spelling; naming blocks and `nameIt` still compose around those verbatim names. Generated tag APIs are omitted when a fixture declares no tags. Tag metadata stays in the parsed-manifest model and is not copied into Runtime's step-matching definitions. If a final generated C++ type name collides with another fixture or manifest type, generation rejects it with a diagnostic instead of emitting invalid C++.

[`Examples/Good/UserTags`](Examples/Good/UserTags) demonstrates validating references against names extracted from tagged data-table steps using typed Args. [`Examples/Bad/UserTags`](Examples/Bad/UserTags) uses the same definitions and rejects a scenario that references an unknown user.
[`Examples/Good/TagNaming`](Examples/Good/TagNaming) verifies that a CMake fixture name remains verbatim under snake_case generated-type conventions.

Diagnostics carry a severity of `Error`, `Warning`, or `Info`; only `Error` prevents the scenario from being accepted, so `Warning`/`Info` are useful for flagging smells (an empty `Examples` table, a step that's a no-op) without failing the build.

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

`StepContext` extends `ScenarioContext`, so an `around_step` hook can inspect typed parameters on the current, previous, or next step through each `StepView`. For example, `context.current().getArgs<StepMethod::startWith>()` returns the generated `startWithArgs` aggregate. You may want to wrap your current step into a try catch block and store the exception for review by the next step and throw only if the next step performs no such assertion.

## Stylesheet naming

The optional stylesheet groups generated-name casing and composition under `cppConventions.types`, `methods`, and `variables`. Each section's `casing` applies to its generated base name; blocks and `nameIt` still compose around that name. Methods can distinguish step definitions from hooks, and variable blocks can distinguish arrays from map key/value types:

```yaml
cppConventions:
  types:
    casing: PascalCase
  methods:
    casing: camelCase
    blocks:
      - blockName: methodKind
        onStepDefinition: step_
        onHook: hook_
    nameIt: [methodKind, methodName]
  variables:
    casing: snake_case
    mapBaseNameIsSingular: true
    blocks:
      - blockName: kindPrefix
        onString: s
      - blockName: mapPrefix
        onMap: a
        onNotMap: ""
      - blockName: mapKey
        onMapKeyType: kindPrefix
      - blockName: mapValue
        onMapValueType: kindPrefix
    nameIt: [mapPrefix, mapKey, mapValue, variableName]
```

`onMapKeyType` and `onMapValueType` reference other variable blocks, which are evaluated using the corresponding map key/value kind. `mapBaseNameIsSingular` selects `additionalProperty` or `additionalProperties` before casing and naming blocks are applied. It affects only the generated additional-properties member. Manifest `type: list` remains the spelling for list fields; `onArray`/`onNotArray` describe their naming-rule category. Undefined-step suggestions use the same section casing, without applying custom naming blocks to user-authored manifest names.

Within `types`, `onClass` applies to generated fixture and interface wrapper classes; manifest structs and enums use `onStruct` and `onEnum`; per-method Args aggregates use `onArgsType`. Args names start from the operation id plus `Args`. Imported or verbatim C++ types keep their user-supplied names and bypass generated type naming.

## CMake integration

Dependency configuration status messages are hidden by default; warnings and errors remain visible. Configure with `-DZUCCHINI_VERBOSE_DEPENDENCIES=ON` to see upstream feature summaries and optional dependency checks, or select an explicit `CMAKE_MESSAGE_LOG_LEVEL`.

```cmake
add_executable(MyTests)

zucchinify(MyTests
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE MyTests
    # Optional:
    # STYLESHEET "${CMAKE_CURRENT_SOURCE_DIR}/features/MyTests.stylesheet.yaml"
)
```

`zucchinify()` expects exactly one YAML manifest in `FEATURE_DIR` (excluding the explicitly supplied `STYLESHEET`). It generates a GTest-free fixture header and a GTest-owning `.cc`, adds them to the target, links `Zucchini::Runtime` and GTest directly, and lets `gtest_discover_tests` enumerate dynamically registered scenarios. Changes to the manifest, optional stylesheet, generator, or templates regenerate the fixture sources and rebuild the affected objects. Editing, adding, or removing feature files only relinks the test executable and reruns discovery; it does not regenerate or recompile fixture sources unless discovery updates the manifest with missing definitions, which triggers regeneration on the next build. The default discovery mode is `POST_BUILD`; `DISCOVERY_MODE PRE_TEST` defers rediscovery until CTest runs and is exercised by `Examples/Good/PreTestDiscovery` and `CMakeIntegration.PreTestDiscovery`.

`zucchinify()` accepts `SNIPPET_MODE MERGE` (the default) or `SNIPPET_MODE PRINT`. In `PRINT` mode, discovery prints inferred YAML snippets to stderr and leaves the source manifest unchanged. This is useful, for example, in CI, where missing definitions should appear in the build log without modifying the checkout. Missing definitions still cause discovery to fail.

In `MERGE` mode, when discovery finds missing step definitions, the runtime inserts inferred definitions directly into that source manifest. Proposed entries use block-style YAML, with blank lines between array elements. Existing block entries remain unchanged. Flow collections are expanded into block collections where needed, retaining existing entry text, comments, and line endings; libfyaml supplies the source positions. The complete candidate is schema-validated before replacing the file. Discovery then fails with an absolute `vscode://file/...:1:1` URL so you can review the definitions and implement the fixture methods before rebuilding. Repeated discovery from an older executable does not duplicate entries. If an update fails, discovery retains the copy-paste suggestions. Direct executable discovery without the `step_definitions` argument also retains suggestions.


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
├── LibZucchini/      shared definitions, manifest parsing and validation
├── ZucchiniRuntime/  generated-test runtime, discovery and manifest editing
├── Zucchini/         command-line compiler and lowering into generated C++
├── cmake/            CMake integration (zucchinify())
├── schemas/          manifest and stylesheet schemas
├── scripts/          repository tooling
├── Examples/
│   ├── Good/          working end-to-end examples
│   └── Bad/           expected generator/discovery failures
└── CMakeLists.txt
```

`LibZucchini` owns the definitions, diagnostics, YAML manifest parsing, and schema validation shared by the generator and runtime. The `Zucchini` executable owns stylesheet parsing, lowering, and code generation. Generated test targets link `ZucchiniRuntime`, which owns Gherkin discovery, source-preserving manifest updates, and scenario-plan storage and execution. The generator does not link the runtime; manifest updates run directly in the generated test executable.

## Status

I’m using Zucchini for more and more tests in private repositories, so major breaking API changes are becoming less likely. This isn’t a stable API guarantee, but I expect upcoming work to focus on features and bug fixes rather than fundamental changes.
The project’s focus remains deliberately narrow: compiling Gherkin + a typed manifest into native GoogleTest/CTest.

## Contributing

New compiler behavior should usually come with a `Examples/Good` case (successful generation/execution), an `Examples/Bad` case (an expected diagnostic), or both — the examples are part of the compiler's behavioral contract, not just a demo. See [`Examples/README.md`](Examples/README.md) for how they're wired up.

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

## Rules and requirements

A passing test tells you that a scenario worked. Zucchini's rules help you see
whether your scenarios actually check the business requirements you care about.
For example, a refund test might verify that stock is restored but never check
that the customer gets their money back. The rule dashboard makes that gap visible.

Rules also catch problems in the scenarios themselves before tests run: missing
setup, invalid references, or expectations that contradict your business rules.
You can check wallet behavior and inventory behavior independently, keeping each
rule focused as your test suite grows.

The browser dashboard connects requirements to the scenarios that check them
and their latest test results. See what's covered, what's missing, and which
checks pass or fail, with the relevant feature lines alongside the evidence.
Coverage and test success stay separate, so a failing test doesn't masquerade
as proof that a requirement works.

Enable it by adding `RULE_COVERAGE_REPORT` to your `zucchinify()` call. Open the
generated report's `index.html` locally, share the whole report directory as a
CI artifact, or publish it on a static website. The report includes the scenario
source, so readers don't need a checkout to understand the results.

See the [business rules example](Examples/Good/BusinessRules) for a small shop
that demonstrates refund coverage gaps, scenario validation, and failing tests.
A demo of the report associated with this example can be found
[here](https://anarchosystems.github.io/Zucchini/).

## License

MIT. See [`LICENSE`](LICENSE).
