#pragma once
#include "ShopFacts.h"

namespace nShop {
enum class InventoryRequirement {
  PurchaseConsumesStock,
  RejectedPurchaseKeepsStock,
  RefundRestocksItems
};

// This model knows quantities and asserted outcomes. It does not know prices or
// wallet balances, and ignores those facts explicitly; a rejection for any
// reason must preserve inventory.
struct InventoryRule final : BusinessRule<InventoryRequirement> {
  using Reporter = nZucchini::BusinessReporter<InventoryRequirement>;

  nZucchini::RuleMetadata metadata() const override {
    return {"Inventory conservation",
            "Purchases consume items, rejected purchases preserve stock, and "
            "refunds restock items."};
  }
  Requirements requirements() const override {
    return {{InventoryRequirement::PurchaseConsumesStock,
             {"Purchase Consumes Stock",
              "An accepted purchase is followed by an assertion of the exact "
              "inventory decrement.",
              nZucchini::Strictness::Exact}},
            {InventoryRequirement::RejectedPurchaseKeepsStock,
             {"Rejected Purchase Keeps Stock",
              "A rejected purchase is followed by an assertion that inventory "
              "did not change.",
              nZucchini::Strictness::Exact}},
            {InventoryRequirement::RefundRestocksItems,
             {"Refund Restocks Items",
              "A refund is followed by an assertion that the purchased items "
              "were returned to stock.",
              nZucchini::Strictness::Exact}}};
  }
  // Reads facts, never steps; see WalletRule.
  void validate(const StepContext &context, Reporter &reporter) override {
    const auto current = context.current();
    const auto handle = [&](const auto &fact) {
      on(fact, current.step, reporter);
    };
    onFact<StepTag::Setup>(context, handle);
    onFact<StepTag::Transaction>(context, handle);
    onFact<StepTag::Outcome>(context, handle);
    onFact<StepTag::Observation>(context, handle);
  }

private:
  void on(const InitialBalance &, const ZucchiniStep &, Reporter &) {}
  void on(const InitialStock &fact, const ZucchiniStep &, Reporter &) {
    observation.reset();
    expectedStock = fact.items;
  }
  void on(const UnitPrice &, const ZucchiniStep &, Reporter &) {}
  void on(const Purchase &fact, const ZucchiniStep &, Reporter &) {
    requested = fact.quantity;
  }
  void on(const Refund &, const ZucchiniStep &, Reporter &) {
    expectedStock += purchased;
    purchased = 0;
    observation = InventoryRequirement::RefundRestocksItems;
  }
  void on(const Accepted &, const ZucchiniStep &step, Reporter &reporter) {
    observation = InventoryRequirement::PurchaseConsumesStock;
    if (requested > expectedStock) {
      reporter.violation(*observation, nZucchini::DiagnosticSeverity::Error,
                         "An accepted purchase would overdraw inventory", step);
      purchased = 0;
    } else {
      purchased = requested;
      expectedStock -= purchased;
    }
  }
  void on(const Rejected &, const ZucchiniStep &, Reporter &) {
    purchased = 0;
    observation = InventoryRequirement::RejectedPurchaseKeepsStock;
  }
  void on(const ObservedBalance &, const ZucchiniStep &, Reporter &) {}
  void on(const ObservedStock &fact, const ZucchiniStep &step,
          Reporter &reporter) {
    if (!observation) {
      return;
    }
    if (fact.items != expectedStock) {
      reporter.violation(*observation, nZucchini::DiagnosticSeverity::Error,
                         "The scenario asserts " + std::to_string(fact.items) +
                             " items; the business invariant requires " +
                             std::to_string(expectedStock),
                         step);
    } else {
      reporter.coverage(*observation,
                        "The scenario asserts the expected inventory of " +
                            std::to_string(fact.items) + " items",
                        step);
    }
  }

  long expectedStock = 0;
  long requested = 0;
  long purchased = 0;
  std::optional<InventoryRequirement> observation;
};
} // namespace nShop
