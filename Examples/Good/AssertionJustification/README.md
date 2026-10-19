This demo ties assertions to business requirements and reports the ones no
requirement explains.

`BasketRule.h` follows prices and added items. When the basket's contents are
asserted in a table, it reports coverage for each line's item, quantity and
subtotal cell through `table_cell(row, &Line::member)`. The table spells the
subtotal header `total`, one of the field's alternative headers; the generated
helper resolves it. The manifest ignores the `shelf` column, so its cells
assert nothing. Each requirement declares `Strictness::Exact`, because each
check computes the one expected value.

Three asserted facts are not tied to a requirement:

- both `note` cells. The pear's empty note asserts that it has none.
- `But the checkout button is shown`, a presentation detail.

The report's **Statistics** tab lists them. They are advisory: discovery accepts
every scenario. `report.css`, passed to `zucchinify()` as
`RULE_COVERAGE_REPORT_CSS`, changes the report's badge color for exact
requirements. `AssertJustification.cmake` also checks that a scenario of `*`
steps is counted as asserting nothing, and that discovery without the HTML
report still writes `rules.json`.
