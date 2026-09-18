Feature: Tables

  Background:
    Given I start with 0

  Rule: Typed rows

    Scenario: Row first with an optional and a defaulted column
      When I add these entries
        | value | label | scale |
        | 4     | four  | 2     |
        | 5     | five  | 1     |
      Then the result is 13
      And the note is "four,five"

    Scenario: Optional and defaulted columns may be missing entirely
      When I add these entries
        | value |
        | 4     |
        | 5     |
      Then the result is 9
      And the note is ""

    Scenario: Column first
      When I add these entries by column
        | value | 4    | 5    |
        | label | four | five |
        | scale | 2    | 1    |
      Then the result is 13
      And the note is "four,five"

  Rule: Positional rows

    Scenario: Typed rows without a header map by position
      When I add these positional entries
        | 4 | four | 2 |
        | 5 | five | 1 |
      Then the result is 13
      And the note is "four,five"

    Scenario: Dynamic rows without a header stay positional
      When I add these raw numbers
        | 4 | 5 |
        | 6 |   |
      Then the result is 15

  Rule: Additional properties

    Scenario: Unknown columns are collected
      When I add these loose entries
        | value | unit | source |
        | 4     | kg   | scale  |
      Then the result is 4
      And the note is "source=scale,unit=kg"
