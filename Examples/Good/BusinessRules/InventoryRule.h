#pragma once
#include "IShop.h"

namespace nShop {
enum class InventoryRequirement {
  PurchaseConsumesStock,
  RejectedPurchaseKeepsStock,
  RefundRestocksItems
};

// This model knows quantities and asserted outcomes. It does not know prices or
// wallet balances; a rejection for any reason must preserve inventory.
struct InventoryRule final : BusinessRule<InventoryRequirement> {
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
  void validate(
      const StepContext &context,
      nZucchini::BusinessReporter<InventoryRequirement> &reporter) override {
    const auto current = context.current();
    // Depend on the Inventory tag, not on the whole step vocabulary. No
    // default: a step newly tagged Inventory fails the build until handled.
    if (!context.tags(current.step).contains(StepTag::Inventory)) {
      return;
    }
    switch (context.cast<StepTag::Inventory>(current.method)) {
    case InventorySteps::stockHas:
      observation.reset();
      expectedStock = current.getArgs<StepMethod::stockHas>().value;
      break;
    case InventorySteps::buy:
      requested = current.getArgs<StepMethod::buy>().quantity;
      break;
    case InventorySteps::purchaseAccepted:
      observation = InventoryRequirement::PurchaseConsumesStock;
      if (requested > expectedStock) {
        reporter.violation(*observation, nZucchini::DiagnosticSeverity::Error,
                           "An accepted purchase would overdraw inventory",
                           current.step);
        purchased = 0;
      } else {
        purchased = requested;
        expectedStock -= purchased;
      }
      break;
    case InventorySteps::purchaseRejected:
      purchased = 0;
      observation = InventoryRequirement::RejectedPurchaseKeepsStock;
      break;
    case InventorySteps::refund:
      expectedStock += purchased;
      purchased = 0;
      observation = InventoryRequirement::RefundRestocksItems;
      break;
    case InventorySteps::stockIs:
      if (observation) {
        const auto asserted = current.getArgs<StepMethod::stockIs>().value;
        if (asserted != expectedStock) {
          reporter.violation(*observation, nZucchini::DiagnosticSeverity::Error,
                             "The scenario asserts " +
                                 std::to_string(asserted) +
                                 " items; the business invariant requires " +
                                 std::to_string(expectedStock),
                             current.step);
        } else {
          reporter.coverage(*observation,
                            "The scenario asserts the expected inventory of " +
                                std::to_string(asserted) + " items",
                            current.step);
        }
      }
      break;
    }
  }

private:
  long expectedStock = 0;
  long requested = 0;
  long purchased = 0;
  std::optional<InventoryRequirement> observation;
};
} // namespace nShop
