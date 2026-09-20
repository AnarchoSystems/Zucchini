Feature: Malformed tables

  Scenario: An unterminated row is rejected
    When I add these entries
      | value | label |
      | 1     | broken