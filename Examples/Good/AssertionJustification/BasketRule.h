#pragma once
#include "IBasket.h"

#include <map>
#include <string>

namespace nBasket {
enum class BasketRequirement { LineListsAddedItem, LineSubtotal, BasketTotal };

// Justifies the basket's contents and prices. Notes and the checkout button are
// presentation details no requirement asks for.
struct BasketRule final : BusinessRule<BasketRequirement> {
  nZucchini::RuleMetadata metadata() const override {
    return {"Basket pricing",
            "Basket lines list added items, and prices add up."};
  }
  Requirements requirements() const override {
    // Each check computes the one expected value from the scenario's prices
    // and additions.
    return {{BasketRequirement::LineListsAddedItem,
             {"Line Lists Added Item",
              "Each basket line names an added item and its quantity.",
              nZucchini::Strictness::Exact}},
            {BasketRequirement::LineSubtotal,
             {"Line Subtotal",
              "Each basket line costs its price times its quantity.",
              nZucchini::Strictness::Exact}},
            {BasketRequirement::BasketTotal,
             {"Basket Total",
              "The basket total is the price of everything added.",
              nZucchini::Strictness::Exact}}};
  }
  void validate(const StepContext &context,
                nZucchini::BusinessReporter<BasketRequirement> &reporter)
      override {
    const auto current = context.current();
    switch (current.method) {
    case StepMethod::priceIs: {
      const auto args = current.getArgs<StepMethod::priceIs>();
      prices[args.item] = args.price;
    } break;
    case StepMethod::addToBasket: {
      const auto args = current.getArgs<StepMethod::addToBasket>();
      quantities[args.item] += args.quantity;
    } break;
    case StepMethod::basketContains: {
      const auto rows = current.getArgs<StepMethod::basketContains>().rows;
      for (std::size_t row = 0; row < rows.size(); ++row) {
        check_line(rows[row], row, current.step, reporter);
      }
    } break;
    case StepMethod::totalIs: {
      long expected = 0;
      for (const auto &[item, quantity] : quantities) {
        expected += price_of(item) * quantity;
      }
      if (current.getArgs<StepMethod::totalIs>().value == expected) {
        reporter.coverage(BasketRequirement::BasketTotal,
                          "The total adds up the basket", current.step);
      } else {
        reporter.violation(BasketRequirement::BasketTotal,
                           nZucchini::DiagnosticSeverity::Error,
                           "The basket costs " + std::to_string(expected) +
                               " credits",
                           current.step);
      }
    } break;
    default:
      break;
    }
  }

private:
  long price_of(const std::string &item) const {
    const auto price = prices.find(item);
    return price == prices.end() ? 0 : price->second;
  }

  void check_line(const Line &line, std::size_t row, const ZucchiniStep &step,
                  nZucchini::BusinessReporter<BasketRequirement> &reporter) {
    const auto added = quantities.find(line.item);
    if (added == quantities.end()) {
      reporter.violation(BasketRequirement::LineListsAddedItem,
                         nZucchini::DiagnosticSeverity::Error,
                         "Nothing added is called '" + line.item + "'", step,
                         table_cell(row, &Line::item));
      return;
    }
    reporter.coverage(BasketRequirement::LineListsAddedItem,
                      "The line names an added item", step,
                      table_cell(row, &Line::item));
    if (line.quantity == added->second) {
      reporter.coverage(BasketRequirement::LineListsAddedItem,
                        "The line shows the added quantity", step,
                        table_cell(row, &Line::quantity));
    } else {
      reporter.violation(BasketRequirement::LineListsAddedItem,
                         nZucchini::DiagnosticSeverity::Error,
                         std::to_string(added->second) + " were added", step,
                         table_cell(row, &Line::quantity));
    }
    if (line.subtotal == price_of(line.item) * line.quantity) {
      reporter.coverage(BasketRequirement::LineSubtotal,
                        "The subtotal is price times quantity", step,
                        table_cell(row, &Line::subtotal));
    } else {
      reporter.violation(BasketRequirement::LineSubtotal,
                         nZucchini::DiagnosticSeverity::Error,
                         "The subtotal must be price times quantity", step,
                         table_cell(row, &Line::subtotal));
    }
  }

  std::map<std::string, long> prices;
  std::map<std::string, long> quantities;
};
} // namespace nBasket
