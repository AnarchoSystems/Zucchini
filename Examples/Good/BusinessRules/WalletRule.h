#pragma once
#include "ShopFacts.h"
#include <limits>

namespace nShop {
enum class WalletRequirement {
  PurchaseDebitsPrice,
  RejectedPurchaseKeepsBalance,
  RefundRestoresBalance
};

// This model knows wallet effects, prices, and asserted outcomes. It does not
// know how much inventory exists or why the shop might reject a purchase; it
// ignores those facts explicitly.
struct WalletRule final : BusinessRule<WalletRequirement> {
  using Reporter = nZucchini::BusinessReporter<WalletRequirement>;

  nZucchini::RuleMetadata metadata() const override {
    return {"Wallet conservation",
            "Purchases debit their price, rejected purchases preserve credits, "
            "and refunds restore them."};
  }
  Requirements requirements() const override {
    return {
        {WalletRequirement::PurchaseDebitsPrice,
         {"Purchase Debits Price", "An accepted purchase is followed by an "
                                 "assertion of the exact wallet debit.",
          nZucchini::Strictness::Exact}},
        {WalletRequirement::RejectedPurchaseKeepsBalance,
         {"Rejected Purchase Keeps Balance",
          "A rejected purchase is followed by an assertion that wallet credits "
          "did not change.",
          nZucchini::Strictness::Exact}},
        {WalletRequirement::RefundRestoresBalance,
         {"Refund Restores Balance", "A refund is followed by an assertion that "
                                   "the debited credits were restored.",
          nZucchini::Strictness::Exact}}};
  }
  // The rule reads facts, never steps: it depends on the fact types in
  // ShopFacts.h, not on the step vocabulary. Each fact alternative needs an
  // on() overload; ignoring one is an explicit, empty overload.
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
  void on(const InitialBalance &fact, const ZucchiniStep &, Reporter &) {
    observation.reset();
    expectedBalance = fact.credits;
  }
  void on(const InitialStock &, const ZucchiniStep &, Reporter &) {}
  void on(const UnitPrice &fact, const ZucchiniStep &, Reporter &) {
    price = fact.credits;
  }
  void on(const Purchase &fact, const ZucchiniStep &, Reporter &) {
    pendingCharge =
        fact.quantity > 0 &&
                price <= std::numeric_limits<long>::max() / fact.quantity
            ? std::optional<long>(price * fact.quantity)
            : std::nullopt;
  }
  void on(const Refund &, const ZucchiniStep &, Reporter &) {
    expectedBalance += debited;
    debited = 0;
    observation = WalletRequirement::RefundRestoresBalance;
  }
  void on(const Accepted &, const ZucchiniStep &step, Reporter &reporter) {
    observation = WalletRequirement::PurchaseDebitsPrice;
    if (!pendingCharge || *pendingCharge > expectedBalance) {
      reporter.violation(*observation, nZucchini::DiagnosticSeverity::Error,
                         "An accepted purchase would overdraw the wallet",
                         step);
      debited = 0;
    } else {
      debited = *pendingCharge;
      expectedBalance -= debited;
    }
  }
  void on(const Rejected &, const ZucchiniStep &, Reporter &) {
    debited = 0;
    observation = WalletRequirement::RejectedPurchaseKeepsBalance;
  }
  void on(const ObservedBalance &fact, const ZucchiniStep &step,
          Reporter &reporter) {
    if (!observation) {
      return;
    }
    if (fact.credits != expectedBalance) {
      reporter.violation(*observation, nZucchini::DiagnosticSeverity::Error,
                         "The scenario asserts " +
                             std::to_string(fact.credits) +
                             " credits; the business invariant requires " +
                             std::to_string(expectedBalance),
                         step);
    } else {
      reporter.coverage(*observation,
                        "The scenario asserts the expected wallet balance of " +
                            std::to_string(fact.credits) + " credits",
                        step);
    }
  }
  void on(const ObservedStock &, const ZucchiniStep &, Reporter &) {}

  long expectedBalance = 0;
  long price = 0;
  std::optional<long> pendingCharge;
  long debited = 0;
  std::optional<WalletRequirement> observation;
};
} // namespace nShop
