Feature: Prepaid shop
  A purchase spends wallet credits and consumes inventory.
  A rejected purchase changes neither. A refund reverses both effects.

  Background:
    Given each item costs 7 credits

  Rule: Purchases preserve money and stock

    Scenario: Buying two items
      Given my wallet has 50 credits
      And the shop has 5 items
      When I buy 2 items
      Then the purchase is accepted
      And my wallet balance is 36 credits
      And the remaining stock is 3

    Scenario: Insufficient funds change nothing
      Given my wallet has 5 credits
      And the shop has 10 items
      When I buy 1 items
      Then the purchase is rejected
      And my wallet balance is 5 credits
      And the remaining stock is 10

    Scenario: Sold out purchases change nothing
      Given my wallet has 50 credits
      And the shop has 0 items
      When I buy 1 items
      Then the purchase is rejected
      And my wallet balance is 50 credits
      And the remaining stock is 0

  Rule: Refunds restore the state before the purchase

    @coverage_gap
    Scenario: Refund checked for stock but not wallet balance
      Given my wallet has 50 credits
      And the shop has 5 items
      When I buy 2 items
      Then the purchase is accepted
      And my wallet balance is 36 credits
      And the remaining stock is 3
      When I refund the purchase
      Then the remaining stock is 5
      # This test passes, but never asserts that the refund restored the wallet.
