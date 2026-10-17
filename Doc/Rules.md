# Get started with rules

[Overview](../README.md) · [Installation](Installation.md) · [Working with Zucchini](WorkingWithZucchini.md)

A passing refund test might check that stock is restored but never check that the
customer gets their money back. Rules make that missing assertion visible.
They connect business requirements to the scenarios that check them, and can
reject scenarios whose expectations contradict those requirements.

## Start with the shop example

The [business rules example](../Examples/Good/BusinessRules) models purchases and
refunds with independent wallet and inventory rules. Its ordinary scenarios pass,
but the refund scenario deliberately omits the wallet assertion.

1. [Build and test the repository](Installation.md#try-the-examples).
2. Open the generated `index.html` under
   `build/Examples/Good/BusinessRules/zucchini-reports/Shop/<configuration>/`.
   For a Release build, `<configuration>` is `Release`.
3. Find the uncovered **Refund Restores Balance** requirement.
4. Add `And my wallet balance is 50 credits` after the refund in
   [Shop.feature](../Examples/Good/BusinessRules/features/Shop.feature).
5. Rebuild and rerun the tests, then refresh the report to see the added evidence.

You can also [browse the published dashboard](https://anarchosystems.github.io/Zucchini/)
without building the example. The example's [guide](../Examples/Good/BusinessRules/README.md)
explains its deliberately rejected and failing scenarios.

## Add rules to your fixture

Rules inspect the scenario's typed steps during discovery, before the fixture's
step methods execute. They describe what a scenario should assert; the step
implementations and GoogleTest assertions still check the actual software.

### Declare a focused business rule

Give each obligation an enum value and a readable description. For example,
[WalletRule.h](../Examples/Good/BusinessRules/WalletRule.h) declares:

```cpp
enum class WalletRequirement {
  PurchaseDebitsPrice,
  RejectedPurchaseKeepsBalance,
  RefundRestoresBalance
};

struct WalletRule final : BusinessRule<WalletRequirement> {
  // metadata() names the rule.
  // requirements() gives each enum value a name and description.
  // validate() inspects typed steps and reports coverage or violations.
};
```

This is the shape of the rule; use the linked implementation for the complete
methods. Include the generated fixture header (`IShop.h` in this example) and
define the rule inside the fixture's namespace (`nShop`).

### Report evidence at the assertion

In `validate()`, use `context.current().method` to identify a step and
`context.current().getArgs<StepMethod::balanceIs>()` to read its typed arguments.
Track only the state needed for this rule: the wallet rule follows prices,
purchase outcomes, and expected balance, while the inventory rule follows stock.
Each scenario gets fresh rule instances.

When a balance assertion agrees with the expected balance, the wallet rule calls
`reporter.coverage(requirement, message, current.step)`. If it contradicts the
rule, it calls `reporter.violation(requirement, severity, message, current.step)`.
An error rejects the scenario during discovery; warnings and information remain
visible without rejecting it.

Emit coverage when the scenario checks the required effect. Performing a refund
alone does not show that the scenario checks the returned money. Keeping the
requirement declaration separate from coverage lets the report show obligations
that no scenario checks yet.

### Register the rules

Override `register_rules()` in your fixture. The shop registers three independent
rules:

```cpp
void register_rules(RuleRegistry &registry) override {
  registry.add<PurchaseSequenceRule>();
  registry.add<WalletRule>();
  registry.add<InventoryRule>();
}
```

See [Shop.h](../Examples/Good/BusinessRules/Shop.h) for the fixture and includes.
Use `BusinessRule<Requirement>` for requirements with coverage evidence.
Use `DSLRule<Constraint>` for scenario structure, such as setup ordering or an
outcome assertion required after a purchase. DSL rules report violations without
claiming business coverage; an `on_scenario_end()` hook can catch unfinished
sequences. See [PurchaseSequenceRule.h](../Examples/Good/BusinessRules/PurchaseSequenceRule.h).

## Enable the report

Add `RULE_COVERAGE_REPORT` to the fixture's CMake setup:

```cmake
add_executable(BusinessRulesTest)
zucchinify(BusinessRulesTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE Shop
    RULE_COVERAGE_REPORT
)
zucchini_add_main(BusinessRulesTest Shop)
```

By default, reports are written under
`<target build directory>/zucchini-reports/<fixture>/<configuration>/`.
Use `RULE_COVERAGE_REPORT_PATH` to choose another location.
Build to discover scenarios and their rule observations. Test execution does not
read or write rule reports.

## Justify every assertion

Coverage shows that each requirement has an assertion. The reverse matters too:
an assertion no requirement asks for may pin down incidental behavior. Opt in
to check that every asserted fact is justified by business rule coverage:

```cmake
zucchinify(BasketTest
    FEATURE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/features"
    FIXTURE Basket
    RULE_COVERAGE_REPORT
    UNJUSTIFIED_ASSERTION_COVERAGE
    UNJUSTIFIED_ASSERTION_SEVERITY WARNING            # ERROR, WARNING (default) or INFO
    MAX_UNJUSTIFIED_ASSERTED_FACTS_PER_SCENARIO 2     # default 2
)
```

Gherkin classifies each step's keyword in the feature's language. Steps of the
outcome type (`Then`, and the `And` or `But` steps that follow it) are
assertions; `*` makes no such claim. An assertion's facts are its step or, for a
data table, each cell, including empty ones: an empty cell may assert absence.
Columns the manifest declares with `type: ignore` assert nothing. DocStrings are
opaque, so the step is their fact. A fact is justified when a business rule
reports coverage for it; violations do not justify.

With the check enabled, discovery reports each step with unjustified facts at
the chosen severity; an error rejects the scenario. Whatever the severity, it
rejects scenarios that assert nothing and scenarios with more unjustified facts
than the maximum. Choosing a severity or maximum implies the check.
Independently of the check, `RULE_COVERAGE_REPORT` records each scenario's
unjustified facts, so the dashboard and the editor extension can show them.

Rules justify table cells by naming them. Generated fixtures provide
`table_cell(row, &Struct::member)` for typed rows; it resolves whichever of the
field's headers the table uses. `nZucchini::TableCell(row, "header")` and
`TableCell(row, position)` name cells of dynamic tables. Coverage of a table
step without a cell justifies no cells:

```cpp
const auto rows = current.getArgs<StepMethod::basketContains>().rows;
for (std::size_t row = 0; row < rows.size(); ++row)
  reporter.coverage(BasketRequirement::LineSubtotal, "Price times quantity",
                    current.step, table_cell(row, &Line::subtotal));
```

See the [assertion justification example](../Examples/Good/AssertionJustification),
where the basket rule leaves a free-form note and a presentation check
unjustified.

## Read and share the evidence

The dashboard shows requirements, the scenarios that check them, their source,
and optionally imported CTest execution results. Export them with
`ctest --test-dir build -C Debug --output-junit results.xml`, then select that XML
file using **Load CTest results** in the dashboard. Use the configuration you built.
The file picker works when opening the HTML locally, too.
For automatic loading beside the HTML, run this once after CTest (also when tests
fail):

```sh
python scripts/export_ctest_results.py build/results.xml path/to/report
```

This publishes `ctest-results.js`; the dashboard loads it automatically using a
local script, just like discovery data. No test process writes report files. Import results from the
same build: JUnit does not carry a discovery fingerprint. Reimport after each run;
scenarios absent from the file show no imported result. The importer recognizes
the default CTest names generated by Zucchini; custom test prefixes or suffixes
are not matched. Coverage and success are separate: discovery
can recognize a meaningful assertion even when execution later fails because the
implementation is wrong.

Open `index.html` locally, share the whole report directory as a CI artifact, or
publish it on a static website. The report includes scenario source so readers
do not need a checkout.

For the same workflow inside your editor, install the
[Zucchini Rules VS Code extension](../ZucchiniVSCode/README.md). Right-click a fixture to
[write another rule with an AI agent](../ZucchiniVSCode/README.md#write-a-rule-with-ai),
or draft examples and counterexamples using an existing rule and implementation
context.
The [report schema documentation](../schemas/README.md) covers the JSON format
and compatibility policy for tool authors.
