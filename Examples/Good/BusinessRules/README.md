This demo models a prepaid shop. An accepted purchase debits the wallet and
consumes inventory; a rejection preserves both; a refund restores both.

`Shop.h` implements the shop and registers three independent rules:

- `WalletRule.h` tracks prices and wallet effects. It ignores inventory.
- `InventoryRule.h` tracks quantities and inventory effects. It ignores prices
  and wallet balances.
- `PurchaseSequenceRule.h` checks setup, purchase outcome assertions, and refund
  ordering. Its enum-tagged constraints provide descriptions when violated; it
  emits no coverage observations. Its end hook detects an unfinished purchase protocol.

Business coverage is emitted by the balance or stock assertion that verifies an
expected postcondition. Merely performing a purchase or refund emits no coverage.
The models use the scenario's asserted acceptance/rejection outcome to predict
its effects; they do not reproduce the shop's complete eligibility policy.

The ordinary feature has four passing scenarios. The refund scenario asserts
stock restoration but omits the wallet assertion. Its dashboard therefore has
five requirements with passing evidence and an uncovered `Refund Restores Balance`
requirement. Add `And my wallet balance is 50 credits` after the refund to close
that gap. Rule instances are fresh for every scenario; no begin hook resets state.

With the default build directory, open:

`build/Examples/Good/BusinessRules/zucchini-reports/Shop/<configuration>/index.html`

Use Microsoft Live Preview for an interactive view inside VS Code. Scenarios render inline with rule observations on hover. Source locations are
plain file/line references relative to the features directory. The scenarios view
has a separate section for scenarios without recorded rule observations.
CI uploads the complete report folders as downloadable artifacts; you can also publish the report directory on any static host.

`Rules.BusinessInvariantDemo` also generates isolated, labelled example reports
under `report-test/<configuration>`:

- `passing/index.html`: the passing suite with its intentionally untested wallet
  refund invariant.
- `rejected/index.html`: an incorrect balance assertion rejected by the wallet
  business rule during discovery.
- `unfinished/index.html`: a scenario rejected by the DSL rule's end hook
  because it never declares the purchase outcome.
- `failing-evidence/index.html`: a scenario that expects rejection despite ample
  funds and stock. Its conditional balance/stock expectations are coherent with
  the declared outcome, so discovery records coverage; execution fails because
  the shop accepts the purchase. This shows why static coverage and runtime
  success are separate dimensions.

Expected failures are checked by the CMake test script; they do not make the
project's ordinary test suite fail.
