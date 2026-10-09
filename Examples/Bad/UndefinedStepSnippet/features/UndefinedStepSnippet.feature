Feature: Adding items to a cart

  Scenario: A simple order
    When I add the following items:
      | name   | unit price | quantity | in stock |
      |        |            |          |          |
      | Widget | 2.5        | 3        | true     |
    Then the cart total is 2.5
    And the receipt contains "Thank you for your order"

  Scenario: An order with a discount
    When I add the following items:
      | name | unit price | quantity | in stock | discount |
      | Gear | 5          | 1        | false    | 10       |

  Scenario: Optional scalar cells keep their observed types
    When I add the following items:
      | name | unit price | quantity | in stock | discount | memo | unknown |
      | Gear | 5.5        | 2        | yes      | 4        | text |         |
      |      |            |          |          |          |      |         |

  Scenario: Lists are inferred across rows and scenarios
    When I collect values:
      | integers | decimals | flags | words | mixed | Column 1 | Column 2 | Column12 | 3 |
      | 2        | 2.5      | true  | plain | 2     | 1        | 2        | 12       | 3 |
    When I collect values:
      | integers | decimals | flags   | words       | mixed   | Column 1 | Column 2 | Column12 | 3 |
      | 3, -4    | 3, 4.5   | YES, NO | alpha, beta | 3, text | 4        | 5        | 6        | 7 |
      |          |          |         |             |         |          |          |          |   |
