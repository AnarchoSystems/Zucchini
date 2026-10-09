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

  Scenario: Empty optional custom strings are absent
    Then these optional nicknames are absent
      | name  | age | nickname |
      | Alice | 30  |          |

  Scenario: Missing optional custom strings are absent
    Then these optional nicknames are absent
      | name  | age |
      | Alice | 30  |

  Scenario: Custom string list elements are decoded from cells
    Then these custom string lists are decoded
      | names      | optionalNames |
      | Alice, Bob |               |

  Scenario: Custom string lists round trip through JSON
    Then these custom string lists are decoded from json
      """json
      {"names":["Alice","Bob"],"optionalNames":null}
      """
