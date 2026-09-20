Feature: Typed doc strings

  Scenario: Malformed JSON is rejected
    When I note this json
      """json
      { nope
      """