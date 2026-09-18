Feature: Calculator

  Rule: Addition

    Scenario: Adding two numbers
      Given I start with 1
      When I add 2
      Then the result is 3

    Scenario: Adding a column of numbers
      Given I start with 0
      When I add these numbers
        | value |
        | 4     |
        | 5     |
      Then the result is 9

  Rule: Subtraction

    Scenario: Subtracting by adding a negative number
      Given I start with 10
      When I add -4
      Then the result is 6
