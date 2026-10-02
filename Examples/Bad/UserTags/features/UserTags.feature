Feature: Tagged user references

  Scenario: An unknown user is referenced
    Given users are introduced
      | name  |
      | Alice |
    When user "Eve" is referenced