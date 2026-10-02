Feature: Tagged user references

  Scenario: A referenced user was introduced
    Given users are introduced
      | name  |
      | Alice |
      | Bob   |
    When user "Bob" is referenced