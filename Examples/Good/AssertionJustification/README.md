This demo checks that every assertion is justified by a business rule.

`BasketRule.h` follows prices and added items. When the basket's contents are
asserted in a table, it reports coverage for each line's item, quantity and
subtotal cell through `table_cell(row, &Line::member)`. The table spells the
subtotal header `total`, one of the field's alternative headers; the generated
helper resolves it. The manifest ignores the `shelf` column, so its cells
assert nothing.

`CMakeLists.txt` enables `UNJUSTIFIED_ASSERTION_COVERAGE` with its defaults:
unjustified facts are warnings, and each scenario may assert at most two of
them. Discovery warns about three facts no requirement asks for:

- both `note` cells. The pear's empty note asserts that it has none.
- `But the checkout button is shown`, a presentation detail.

No scenario exceeds the limit, so discovery succeeds. `AssertJustification.cmake`
also checks that `--unjustified-assertions=error` rejects the second scenario,
that `--max-unjustified-facts-per-scenario=1` rejects the first, and that a
scenario of `*` steps fails because it asserts nothing.
