Feature: Basket
  Every assertion should check something a business rule asks for. The basket
  rule justifies items, quantities and subtotals, but not free-form notes or the
  checkout button. The manifest ignores shelves, so they assert nothing.

  Background:
    Given each apple costs 3 credits
    And each pear costs 5 credits

  Scenario: Basket lines show what was added
    When I put 2 apple into my basket
    And I put 1 pear into my basket
    Then my basket contains
      | item  | quantity | total | note    | shelf |
      | apple | 2        | 6     | crunchy | A3    |
      | pear  | 1        | 5     |         | B1    |
    And my basket total is 11 credits

  Scenario: The basket total sums its lines
    When I put 3 pear into my basket
    Then my basket total is 15 credits
    But the checkout button is shown
