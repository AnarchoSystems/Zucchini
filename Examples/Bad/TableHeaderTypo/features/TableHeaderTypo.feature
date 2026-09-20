Feature: Table header typos

  Scenario: A misspelled required header is rejected
    When I add these entries
      | vaule | label |
      | 4     | typo  |