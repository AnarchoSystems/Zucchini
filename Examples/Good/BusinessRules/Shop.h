#pragma once
#include "InventoryRule.h"
#include "PurchaseSequenceRule.h"
#include "WalletRule.h"

namespace nShop {
class Shop : public IShop {
public:
  void register_rules(RuleRegistry &registry) override {
    registry.add<PurchaseSequenceRule>();
    registry.add<WalletRule>();
    registry.add<InventoryRule>();
  }
  void walletHas(long value) override { balance = value; }
  void stockHas(long value) override { stock = value; }
  void itemCosts(long value) override { price = value; }
  void buy(long quantity) override {
    accepted = quantity > 0 && quantity <= stock && price <= balance / quantity;
    purchased = accepted ? quantity : 0;
    charged = purchased * price;
    balance -= charged;
    stock -= purchased;
  }
  void purchaseAccepted() override { EXPECT_TRUE(accepted); }
  void purchaseRejected() override { EXPECT_FALSE(accepted); }
  void balanceIs(long value) override { EXPECT_EQ(value, balance); }
  void stockIs(long value) override { EXPECT_EQ(value, stock); }
  void refund() override {
    balance += charged;
    stock += purchased;
    charged = 0;
    purchased = 0;
  }

private:
  long balance = 0;
  long stock = 0;
  long price = 0;
  long charged = 0;
  long purchased = 0;
  bool accepted = false;
};
} // namespace nShop
