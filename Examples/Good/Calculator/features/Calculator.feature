Feature: Arithmetic

  Background:
    Given I reset the calculation

  Rule: Addition

    Scenario: Adding two numbers
      Given I start with 1
      When I add 2
      Then the result is 3

    Scenario: Adding several numbers
      Given I start with 10
      When I add 5
      And I add 7
      And I add -2
      Then the result is 20

    Scenario Outline: Adding different operands
      Given I start with <start>
      When I add <amount>
      Then the result is <result>

      Examples:
        | start | amount | result |
        | 0     | 0      | 0      |
        | 10    | 5      | 15     |
        | -10   | 7      | -3     |
        | 100   | -25    | 75     |

  Rule: Subtraction

    Scenario: Subtracting a positive number
      Given I start with 10
      When I subtract 4
      Then the result is 6

    Scenario: Subtraction can cross zero
      Given I start with 3
      When I subtract 8
      Then the result is -5

    Scenario Outline: Subtracting different operands
      Given I start with <start>
      When I subtract <amount>
      Then the result is <result>

      Examples:
        | start | amount | result |
        | 0     | 1      | -1     |
        | 10    | 10     | 0      |
        | -5    | 3      | -8     |
