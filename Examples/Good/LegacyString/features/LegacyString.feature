Feature: Legacy string class

  Scenario: Arguments are decoded as the custom string class
    Given I add person "Alice" aged 30
    Then the last name is "Alice"

  Scenario: Data table fields are decoded as the custom string class
    Given these people exist
      | name  | age |
      | Alice | 30  |
      | Bob   | 40  |
    Then the person count is 2
    And the last name is "Bob"
