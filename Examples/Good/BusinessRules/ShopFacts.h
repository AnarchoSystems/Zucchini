#pragma once
#include "IShop.h"

#include <stdexcept>
#include <variant>

// Facts: what a single step states, read from that step alone.
//
// Each manifest tag names exactly one fact and has exactly one extractor,
// `extractFact(<Tag>Steps, const ZucchiniStep &)`. The extractors are the only
// code that knows which steps carry a tag: each switches over its tag's
// generated enum without a default, so tagging a new step fails the build
// (-Werror=switch) until its extractor says which fact the step states. Rules
// never see steps; they consume facts.
//
// Extractors are read-only and use no scenario state. What follows from several
// steps, such as the balance after a purchase, belongs in the rules.
//
// Every fact is a variant. A new alternative, such as a discount among the
// transactions, makes every rule that consumes the fact fail to compile until
// it decides what the alternative means for its requirements.

namespace nShop {

// Tag Setup: the scenario declares part of the shop's initial state.
struct InitialBalance {
  long credits;
};
struct InitialStock {
  long items;
};
struct UnitPrice {
  long credits;
};
using SetupFact = std::variant<InitialBalance, InitialStock, UnitPrice>;

// Tag Transaction: the customer acts.
struct Purchase {
  long quantity;
};
struct Refund {};
using TransactionFact = std::variant<Purchase, Refund>;

// Tag Outcome: the scenario asserts how the shop answered the purchase.
struct Accepted {};
struct Rejected {};
using OutcomeFact = std::variant<Accepted, Rejected>;

// Tag Observation: the scenario asserts part of the shop's resulting state.
struct ObservedBalance {
  long credits;
};
struct ObservedStock {
  long items;
};
using ObservationFact = std::variant<ObservedBalance, ObservedStock>;

[[noreturn]] inline void untagged_step() {
  throw std::logic_error("the step does not carry the extracted tag");
}

inline SetupFact extractFact(SetupSteps method, const ZucchiniStep &step) {
  switch (method) {
  case SetupSteps::walletHas: {
    return InitialBalance{walletHasArgs(step).value};
  }
  case SetupSteps::stockHas: {
    return InitialStock{stockHasArgs(step).value};
  }
  case SetupSteps::itemCosts: {
    return UnitPrice{itemCostsArgs(step).value};
  }
  }
  untagged_step();
}

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
  untagged_step();
}

inline OutcomeFact extractFact(OutcomeSteps method, const ZucchiniStep &) {
  switch (method) {
  case OutcomeSteps::purchaseAccepted: {
    return Accepted{};
  }
  case OutcomeSteps::purchaseRejected: {
    return Rejected{};
  }
  }
  untagged_step();
}

inline ObservationFact extractFact(ObservationSteps method,
                                   const ZucchiniStep &step) {
  switch (method) {
  case ObservationSteps::balanceIs: {
    return ObservedBalance{balanceIsArgs(step).value};
  }
  case ObservationSteps::stockIs: {
    return ObservedStock{stockIsArgs(step).value};
  }
  }
  untagged_step();
}

// Calls `handle` with the fact the current step states under `Tag`, if the step
// carries the tag. `handle` must accept every alternative of the fact.
template <StepTag Tag, typename Handler>
void onFact(const StepContext &context, Handler &&handle) {
  const auto current = context.current();
  if (!context.tags(current.step).contains(Tag)) {
    return;
  }
  std::visit(handle,
             extractFact(context.cast<Tag>(current.method), current.step));
}

} // namespace nShop
