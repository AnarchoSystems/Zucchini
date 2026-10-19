#pragma once
#include "IShop.h"
#include <limits>

namespace nShop {
enum class WalletRequirement {
  PurchaseDebitsPrice,
  RejectedPurchaseKeepsBalance,
  RefundRestoresBalance
};

// This model knows wallet effects, prices, and asserted outcomes. It does not
// know how much inventory exists or why the shop might reject a purchase.
struct WalletRule final : BusinessRule<WalletRequirement> {
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
  void
  validate(const StepContext &context,
           nZucchini::BusinessReporter<WalletRequirement> &reporter) override {
    const auto current = context.current();
    switch (current.method) {
    case StepMethod::walletHas:
      observation.reset();
      expectedBalance = current.getArgs<StepMethod::walletHas>().value;
      break;
    case StepMethod::itemCosts:
      price = current.getArgs<StepMethod::itemCosts>().value;
      break;
    case StepMethod::buy: {
      const auto quantity = current.getArgs<StepMethod::buy>().quantity;
      pendingCharge =
          quantity > 0 && price <= std::numeric_limits<long>::max() / quantity
              ? std::optional<long>(price * quantity)
              : std::nullopt;
    } break;
    case StepMethod::purchaseAccepted:
      observation = WalletRequirement::PurchaseDebitsPrice;
      if (!pendingCharge || *pendingCharge > expectedBalance) {
        reporter.violation(*observation, nZucchini::DiagnosticSeverity::Error,
                           "An accepted purchase would overdraw the wallet",
                           current.step);
        debited = 0;
      } else {
        debited = *pendingCharge;
        expectedBalance -= debited;
      }
      break;
    case StepMethod::purchaseRejected:
      debited = 0;
      observation = WalletRequirement::RejectedPurchaseKeepsBalance;
      break;
    case StepMethod::refund:
      expectedBalance += debited;
      debited = 0;
      observation = WalletRequirement::RefundRestoresBalance;
      break;
    case StepMethod::balanceIs:
      if (observation) {
        const auto asserted = current.getArgs<StepMethod::balanceIs>().value;
        if (asserted != expectedBalance) {
          reporter.violation(*observation, nZucchini::DiagnosticSeverity::Error,
                             "The scenario asserts " +
                                 std::to_string(asserted) +
                                 " credits; the business invariant requires " +
                                 std::to_string(expectedBalance),
                             current.step);
        } else {
          reporter.coverage(
              *observation,
              "The scenario asserts the expected wallet balance of " +
                  std::to_string(asserted) + " credits",
              current.step);
        }
      }
      break;
    default:
      break;
    }
  }

private:
  long expectedBalance = 0;
  long price = 0;
  std::optional<long> pendingCharge;
  long debited = 0;
  std::optional<WalletRequirement> observation;
};
} // namespace nShop
