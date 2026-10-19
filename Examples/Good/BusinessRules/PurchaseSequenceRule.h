#pragma once
#include "IShop.h"

namespace nShop {
enum class PurchaseConstraint {
  SetupBeforePurchase,
  PositivePrice,
  PurchasePreconditions,
  OutcomeFollowsPurchase,
  RefundAcceptedPurchase,
  OutcomeBeforeEffects,
  CompletedPurchase
};
struct PurchaseSequenceRule final : DSLRule<PurchaseConstraint> {
  nZucchini::RuleMetadata metadata() const override {
    return {"Purchase protocol",
            "Set up the shop, declare each purchase outcome, and refund only "
            "an accepted purchase."};
  }
  Requirements requirements() const override {
    return {
        {PurchaseConstraint::SetupBeforePurchase,
         {"Setup before purchase", "Shop setup must precede every purchase."}},
        {PurchaseConstraint::PositivePrice,
         {"Positive price", "Item prices must be positive."}},
        {PurchaseConstraint::PurchasePreconditions,
         {"Purchase preconditions",
          "Set wallet, stock, and positive price; request a positive quantity "
          "with no pending outcome."}},
        {PurchaseConstraint::OutcomeFollowsPurchase,
         {"Outcome follows purchase",
          "Each outcome assertion must follow a pending purchase."}},
        {PurchaseConstraint::RefundAcceptedPurchase,
         {"Refund accepted purchase",
          "Only an accepted purchase can be refunded, once."}},
        {PurchaseConstraint::OutcomeBeforeEffects,
         {"Outcome before effects",
          "Assert the purchase outcome before its effects."}},
        {PurchaseConstraint::CompletedPurchase,
         {"Completed purchase", "Every purchase must have an outcome assertion "
                                "before the scenario ends."}}};
  }
  void validate(
      const StepContext &context,
      nZucchini::ViolationReporter<PurchaseConstraint> &reporter) override {
    const auto current = context.current();
    if (purchaseSeen && (current.method == StepMethod::walletHas ||
                         current.method == StepMethod::stockHas ||
                         current.method == StepMethod::itemCosts)) {
      reporter.violation(PurchaseConstraint::SetupBeforePurchase,
                         nZucchini::DiagnosticSeverity::Error,
                         "Shop setup must precede the purchase", current.step);
    }
    // Deliberately coupled to the whole vocabulary: a protocol must place
    // every step, so this switch covers every StepMethod without a default and
    // any new step fails the build until the protocol decides how it fits.
    // It also catches a step nobody tagged: the business rules, which only
    // consume facts, would never see it.
    switch (current.method) {
    case StepMethod::walletHas:
      walletSet = true;
      break;
    case StepMethod::stockHas:
      stockSet = true;
      break;
    case StepMethod::itemCosts:
      priceSet = true;
      if (current.getArgs<StepMethod::itemCosts>().value == 0) {
        reporter.violation(PurchaseConstraint::PositivePrice,
                           nZucchini::DiagnosticSeverity::Error,
                           "Item prices must be positive", current.step);
      }
      break;
    case StepMethod::buy:
      purchaseSeen = true;
      if (!walletSet || !stockSet || !priceSet || awaitingOutcome ||
          current.getArgs<StepMethod::buy>().quantity == 0) {
        reporter.violation(PurchaseConstraint::PurchasePreconditions,
                           nZucchini::DiagnosticSeverity::Error,
                           "A purchase needs wallet, stock, positive price and "
                           "quantity, and no pending outcome",
                           current.step);
      }
      awaitingOutcome = true;
      break;
    case StepMethod::purchaseAccepted:
    case StepMethod::purchaseRejected:
      if (!awaitingOutcome) {
        reporter.violation(PurchaseConstraint::OutcomeFollowsPurchase,
                           nZucchini::DiagnosticSeverity::Error,
                           "An outcome assertion needs a purchase",
                           current.step);
      }
      awaitingOutcome = false;
      refundable = current.method == StepMethod::purchaseAccepted;
      break;
    case StepMethod::refund:
      if (!refundable || awaitingOutcome) {
        reporter.violation(PurchaseConstraint::RefundAcceptedPurchase,
                           nZucchini::DiagnosticSeverity::Error,
                           "Only an accepted purchase can be refunded, once",
                           current.step);
      }
      refundable = false;
      break;
    case StepMethod::balanceIs:
    case StepMethod::stockIs:
      if (awaitingOutcome) {
        reporter.violation(PurchaseConstraint::OutcomeBeforeEffects,
                           nZucchini::DiagnosticSeverity::Error,
                           "Assert the purchase outcome before its effects",
                           current.step);
      }
      break;
    }
  }
  void on_scenario_end(
      const ScenarioContext &,
      nZucchini::ViolationReporter<PurchaseConstraint> &reporter) override {
    if (awaitingOutcome) {
      reporter.violation(
          PurchaseConstraint::CompletedPurchase,
          nZucchini::DiagnosticSeverity::Error,
          "The scenario ends with a purchase whose outcome was never asserted");
    }
  }

private:
  bool purchaseSeen = false;
  bool walletSet = false;
  bool stockSet = false;
  bool priceSet = false;
  bool awaitingOutcome = false;
  bool refundable = false;
};
} // namespace nShop
