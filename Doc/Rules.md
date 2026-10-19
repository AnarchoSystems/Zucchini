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

> **Keep rules independent of the step vocabulary.** Tag each step with the
> fact it states, write one read-only extractor per tag that switches over the
> tag's generated `<Tag>Steps` enum with **no `default`**, build with
> `-Werror=switch`, and let rules consume only facts. See
> [Decouple rules from the step vocabulary](#decouple-rules-from-the-step-vocabulary).

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
  // validate() consumes the step's facts and reports coverage or violations.
};
```

This is the shape of the rule; use the linked implementation for the complete
methods. Include the generated fixture header (`IShop.h` in this example) and
define the rule inside the fixture's namespace (`nShop`).

### Decouple rules from the step vocabulary

A rule that switches over the fixture's whole `StepMethod` enum is coupled to
the entire step vocabulary. Every new step either breaks it or, behind a
`default:` branch, slips past it silently. The vocabulary grows with the
product, so neither scales, and a rule with a `default:` can stop being correct
without anyone noticing.

The recommended structure has three layers. Zucchini does not enforce it; the
[business rules example](../Examples/Good/BusinessRules) demonstrates it.

**1. A tag names a fact a step states.** Tag a step with what it says, not with
the rules that read it. Whoever declares a step knows what it states, but need
not know which rules exist:

```yaml
steps:
  - group:
      tags: [Transaction]
      steps:
        - step: ^I buy (\d+) items$
          methodName: buy
          # ...
        - step: ^I refund the purchase$
          methodName: refund
  - group:
      tags: [Observation]
      steps:
        - step: ^my wallet balance is (\d+) credits$
          methodName: balanceIs
          # ...
```

**2. Each tag has exactly one extractor.** It turns a step carrying the tag into
the tag's fact, reading that step alone: no scenario state, no side effects.
The extractor is the only code that knows which steps carry the tag, so it is
the only place that switches over `<Tag>Steps`, with no `default`. Generated
`<method>Args` types construct directly from the step:

```cpp
struct Purchase { long quantity; };
struct Refund {};
using TransactionFact = std::variant<Purchase, Refund>;

inline TransactionFact extractFact(TransactionSteps method,
                                   const ZucchiniStep &step) {
  switch (method) {
  case TransactionSteps::buy: {
    return Purchase{buyArgs(step).quantity};
  }
  case TransactionSteps::refund: {
    return Refund{};
  }
  }
  throw std::logic_error("the step does not carry the Transaction tag");
}
```

If a tag seems to need two different facts, it is two tags. If several rules
would read a tag in slightly different ways, the fact is too coarse: split it
until there is one reading, then let the rules ask different questions about it.

**3. Rules consume facts, never steps.** A small helper filters on the tag,
extracts and visits the fact; see `onFact` in
[ShopFacts.h](../Examples/Good/BusinessRules/ShopFacts.h). The rule supplies one
overload per fact alternative, and ignores one with an explicit empty overload:

```cpp
void validate(const StepContext &context, Reporter &reporter) override {
  const auto current = context.current();
  const auto handle = [&](const auto &fact) { on(fact, current.step, reporter); };
  onFact<StepTag::Transaction>(context, handle);
  onFact<StepTag::Observation>(context, handle);
}
void on(const Purchase &fact, const ZucchiniStep &, Reporter &) { /* ... */ }
void on(const Refund &, const ZucchiniStep &, Reporter &) { /* ... */ }
void on(const ObservedBalance &fact, const ZucchiniStep &step, Reporter &r) { /* ... */ }
void on(const ObservedStock &, const ZucchiniStep &, Reporter &) {}
```

Make a missing switch case a build error:

```cmake
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(CheckoutTest PRIVATE -Werror=switch)
endif()
```

On MSVC, warning C4062 is the counterpart (`/we4062`); Zucchini's own CI only
enforces the GCC/Clang flag. Missing `std::visit` overloads are errors anyway.

The compiler now checks two different kinds of change, each where it belongs:

| Change | What fails to build | Rules touched |
|---|---|---|
| A new step under an existing tag | that tag's extractor | none |
| A new kind of fact, as a variant alternative (a discount among transactions) | every rule consuming that fact | each must decide what it means |

Put growth where you expect it: if a concept will gain variants, make its fact a
variant, so new variants force a review of every rule that reads it.

Two limits remain. A step that states a genuinely new concept gets a new tag and
fact, which no existing rule consumes, so nothing forces a rule to reconsider.
And a step nobody tagged never reaches a business rule at all. A protocol rule
that must place every step, such as one checking step order, covers both: it
switches over the full `StepMethod` without a `default`, so any new step fails
the build until someone decides where it fits. See
[PurchaseSequenceRule.h](../Examples/Good/BusinessRules/PurchaseSequenceRule.h).

For a small fixture, the extractor layer can be skipped: switch over one tag's
`<Tag>Steps` directly in the rule, still without a `default`, as the
[assertion justification example](../Examples/Good/AssertionJustification/BasketRule.h)
does. Its rule then depends on that tag's steps rather than on facts.

For table steps, a fact may need to carry the table cell it came from, such as
a list of `{value, nZucchini::TableCell}`, so that rules can still
[claim table cells](#claim-table-cells).

Never write `default:` in a switch over `StepMethod` or `<Tag>Steps`; it turns
these build errors back into silent omissions.

### Report evidence at the assertion

In `validate()`, consume the facts of the current step as shown above. (Without
the fact layer, `context.current().getArgs<StepMethod::balanceIs>()` reads a
step's typed arguments directly.)
Track only the state needed for this rule: the wallet rule follows prices,
purchase outcomes, and expected balance, while the inventory rule follows stock.
Each scenario gets fresh rule instances.

When an observed balance agrees with the expected balance, the wallet rule calls
`reporter.coverage(requirement, message, step)`. If it contradicts the
rule, it calls `reporter.violation(requirement, severity, message, current.step)`.
An error rejects the scenario during discovery; warnings and information remain
visible without rejecting it.

Emit coverage when the scenario checks the required effect. Performing a refund
alone does not show that the scenario checks the returned money. Keeping the
requirement declaration separate from coverage lets the report show obligations
that no scenario checks yet. When the assertion is a data table, report coverage
for the cells you check; see [Claim table cells](#claim-table-cells).

### Declare how strict a requirement is

A requirement can state how narrowly its check pins down the asserted outcome.
Pass a `nZucchini::Strictness` as the third member of its declaration:

```cpp
{WalletRequirement::PurchaseDebitsPrice,
 {"Purchase Debits Price", "An accepted purchase debits its exact price.",
  nZucchini::Strictness::Exact}}
```

| Strictness | The check… | Examples |
|---|---|---|
| `Exact` | names the one expected outcome | the balance is the old balance minus the price |
| `OneOf` | lists the acceptable outcomes | the order is pending or shipped |
| `Bounded` | limits the outcome on every side, in any number of dimensions | the refund is positive and at most the price paid |
| `OpenEnded` | rules some outcomes out but leaves the rest open | the error message starts with "ERR" |
| `Unspecified` | (the default) | |

Strictness describes one requirement. If a scenario can be checked strictly in
one place and loosely in another, declare two requirements. A check that accepts
every outcome checks nothing, so no level describes it.

Strictness is advisory. Reports and the VS Code extension show it as a badge
beside each requirement, ordered from strong to weak, with `Unspecified` last.
It changes neither highlighting nor discovery.

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

Discovery writes `rules.json` for every zucchinified fixture; the VS Code
extension reads it. Add `RULE_COVERAGE_REPORT` to the fixture's CMake setup for
the HTML report as well:

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
Use `RULE_COVERAGE_REPORT_PATH` to choose another location; it also enables the
HTML report. Turning the HTML report off removes the report files discovery
wrote earlier and keeps `rules.json`.

To restyle the HTML report, pass a stylesheet with `RULE_COVERAGE_REPORT_CSS`.
Relative paths are resolved against the current source directory. Discovery
copies it beside the report, which loads it after its own styles, and reruns
when the stylesheet changes. For example, to recolor strictness badges:

```css
:root {
  --strictness-exact: #c8ecd9;
  --strictness-one-of: #d5e3f7;
  --strictness-bounded: #e4dcfa;
  --strictness-open-ended: #f8e5c4;
  --strictness-unspecified: #e5e8ed;
  --strictness-ink: #202735; /* badge text */
}
```

The report redefines these colors for dark mode inside
`@media (prefers-color-scheme: dark)`; override them there too.
Build to discover scenarios and their rule observations. Test execution does not
read or write rule reports.

## Tie assertions to requirements

Coverage answers whether each requirement has an assertion. The reverse
question matters too: an assertion that no requirement asks for may pin down
incidental behavior, and the scenario breaks when that behavior changes. It may
also point at a tacit requirement that has not been made explicit yet.

### What a scenario asserts

Gherkin classifies each step's keyword in the feature's language. Outcome steps
(`Then`, and the `And` or `But` steps that follow it) are assertions; `*` makes
no such claim. Each assertion states one or more facts:

- A plain step is one fact. DocStrings are opaque, so a step with a DocString
  is one fact too.
- A step with a data table states one fact per cell, including empty cells,
  which may assert absence. Cells in columns the manifest declares with
  `type: ignore` assert nothing.

A fact is tied to a requirement when a business rule reports coverage for it.
Violations do not tie facts to requirements.

### Claim table cells

Rules name the cells they check. Generated fixtures provide
`table_cell(row, &Struct::member)` for typed rows; it finds whichever of the
field's alternative headers the table uses. For dynamic tables, use
`nZucchini::TableCell(row, "header")`, or `TableCell(row, position)` without a
header row. `row` counts the records your step receives, starting at 0:

```cpp
const auto rows = current.getArgs<StepMethod::basketContains>().rows;
for (std::size_t row = 0; row < rows.size(); ++row)
  reporter.coverage(BasketRequirement::LineSubtotal, "Price times quantity",
                    current.step, table_cell(row, &Line::subtotal));
```

Coverage of a table step without a cell claims none of its cells. Naming a
cell the table does not contain, such as an absent optional column, claims
nothing. Violations may name a cell as well; discovery then reports them at
that cell.

## Assertion statistics

Rule reports record every asserted fact and whether a requirement claims it.
The **Statistics** tab of the HTML report, and the **Statistics** view of the
VS Code extension, summarize them for each fixture:

- asserted facts, and how many are tied to a requirement;
- facts not tied to a requirement, by scenario, most first. The report shows
  these scenarios again with only those assertions highlighted; hover them for
  details;
- scenarios without assertions;
- each business requirement's declared strictness and the scenarios it claims.

These statistics are advisory only: no value, ratio or percentage among them
should be used as a hard quality target, and discovery rejects no scenario
because of them.
When no requirement explains why an assertion was made, decide whether a rule
should declare the missing requirement, or whether the scenario checks more than
it needs to. Tying every assertion to a requirement is not a goal in itself:
a weak rule written only to claim an assertion hides the question instead of
answering it.

See the [assertion justification example](../Examples/Good/AssertionJustification).
Its basket rule claims items, quantities and subtotals, but leaves a free-form
note and a presentation check untied.

Earlier versions could enforce justification with `UNJUSTIFIED_ASSERTION_COVERAGE`,
`UNJUSTIFIED_ASSERTION_SEVERITY` and `MAX_UNJUSTIFIED_ASSERTED_FACTS_PER_SCENARIO`.
These options no longer have an effect; `zucchinify()` warns when it sees them.

## Read and share the evidence

The dashboard shows requirements, the scenarios that check them, their source,
and optionally imported CTest execution results. In the source, lines with
coverage are green, violations red and assertions not tied to a requirement amber. Data
tables mark each cell on its own, with ignored columns in gray; a table step is
amber while any of its cells is not tied to a requirement. Hover a line or cell for details. Export them with
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
[Zucchini Rules VS Code extension](../ZucchiniVSCode/README.md). Right-click a rule or
requirement to [generate scenarios with AI](../ZucchiniVSCode/README.md#generate-scenarios-with-ai):
passing examples, failing counterexamples or exploratory scenarios, using the
rule and implementation context.
The [report schema documentation](../schemas/README.md) covers the JSON format
and compatibility policy for tool authors.
