Feature: Typed doc strings

  Scenario: A field with the wrong type is rejected
    When I note this json
      """json
      {
        "title": 42,
        "priority": 3
      }
      """