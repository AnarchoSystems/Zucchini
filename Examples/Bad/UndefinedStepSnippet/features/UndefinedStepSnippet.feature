Feature: Adding items to a cart

  Scenario: A simple order
    When I add the following items:
      | name   | unit price |
      | Widget | 2.5        |

  Scenario: An order with a discount
    When I add the following items:
      | name | unit price | discount |
      | Gear | 5.0        | 10       |
