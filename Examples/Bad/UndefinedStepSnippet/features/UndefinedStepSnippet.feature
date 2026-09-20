Feature: Adding items to a cart

  Scenario: A simple order
    When I add the following items:
      | name   | unit price | quantity | in stock |
      | Widget | 2.5        | 3        | true     |
    Then the cart total is 2.5
    And the receipt contains "Thank you for your order"

  Scenario: An order with a discount
    When I add the following items:
      | name | unit price | quantity | in stock | discount |
      | Gear | 5          | 1        | false    | 10       |
