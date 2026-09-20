Feature: Data tables

  Rule: Named columns

    Scenario: Row data uses an alternative header
      When I add these entries
        | amount | name  | multiplier |
        | 4      | four  | 2          |
        | 5      | five  | 1          |
      Then the total is 13
      And the note is "four,five"

    Scenario: Canonical headers work too
      When I add these entries
        | value | label | scale |
        | 3     | three | 3     |
        | 2     | two   | 4     |
      Then the total is 17
      And the note is "three,two"

    Scenario: Optional and defaulted fields can be omitted
      When I add these entries
        | value |
        | 4     |
        | 5     |
      Then the total is 9
      And the note is ""

  Rule: Enumerations and lists

    Scenario: Enum list values are decoded from a semicolon-separated cell
      When I add these tagged entries
        | value | tags       |
        | 2     | red;green  |
        | 5     | blue       |
      Then the total is 7
      And the tag summary is "red,green,blue"

  Rule: Column-oriented tables

    Scenario: Columns are transposed into typed entries
      When I add these entries by column
        | amount | 4    | 5    |
        | name   | four | five |
        | scale  | 2    | 1    |
      Then the total is 13
      And the note is "four,five"

  Rule: Positional tables

    Scenario: Rows without headers map by field order
      When I add these positional entries
        | 4 | four | 2 |
        | 5 | five | 1 |
      Then the total is 13
      And the note is "four,five"

  Rule: Additional properties

    Scenario: Unknown columns are collected
      When I add these loose entries
        | value | unit | source |
        | 4     | kg   | scale  |
      Then the total is 4
      And the note is "source=scale,unit=kg"
