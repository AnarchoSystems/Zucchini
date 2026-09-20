Feature: Invalid enum values

  Scenario: An unknown enum value is rejected
    When I add these tagged entries
      | value | tags        |
      | 2     | ultraviolet |