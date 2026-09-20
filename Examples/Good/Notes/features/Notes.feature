Feature: Notes

  Background:
    Given I start with 0

  Rule: Notes are independent from arithmetic

    Scenario: Taking a note
      When I note
        """
        remember the milk
        """
      Then the note is "remember the milk"

    Scenario: A note can be replaced
      When I note
        """
        first note
        """
      And I note
        """
        second note
        """
      Then the note is "second note"

  Rule: Scenario hooks

    Scenario: Step execution is wrapped for each step
      When I note
        """
        remember the milk
        """
      Then the wrapped steps are 3

  Rule: Scenario outlines

    Scenario Outline: Arithmetic around a note
      When I add <first>
      And I note
        """
        <message>
        """
      And I add <second>
      Then the result is <total>
      And the note is "<message>"

      Examples:
        | first | message         | second | total |
        | 1     | one plus two    | 2      | 3     |
        | 10    | ten minus three | -3     | 7     |
        | -2    | negative        | 5      | 3     |
