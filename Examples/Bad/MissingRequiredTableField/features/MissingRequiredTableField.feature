Feature: Required table fields

  Scenario: A missing required field is rejected
    When I add these entries
      | label |
      | nope  |