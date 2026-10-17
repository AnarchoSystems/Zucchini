#pragma once
#include "BasketRule.h"

#include <map>
#include <string>

namespace nBasket {
class Basket : public IBasket {
public:
  void register_rules(RuleRegistry &registry) override {
    registry.add<BasketRule>();
  }
  void priceIs(const std::string &item, long price) override {
    prices[item] = price;
  }
  void addToBasket(long quantity, const std::string &item) override {
    contents[item] += quantity;
  }
  void basketContains(const std::vector<Line> &rows) override {
    EXPECT_EQ(rows.size(), contents.size());
    for (const auto &line : rows) {
      EXPECT_EQ(line.quantity, contents[line.item]);
      EXPECT_EQ(line.subtotal, prices[line.item] * line.quantity);
    }
  }
  void totalIs(long value) override {
    long total = 0;
    for (const auto &[item, quantity] : contents) {
      total += prices[item] * quantity;
    }
    EXPECT_EQ(value, total);
  }
  void checkoutShown() override { EXPECT_FALSE(contents.empty()); }

private:
  std::map<std::string, long> prices;
  std::map<std::string, long> contents;
};
} // namespace nBasket
