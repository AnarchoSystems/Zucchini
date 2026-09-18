Feature: Notes

  Background:
    Given I start with 0

  Scenario Outline: Adding twice
    When I add <first>
    And I add <second>
    Then the result is <total>

    Examples:
      | first | second | total |
      | 1     | 2      | 3     |
      | 10    | -3     | 7     |

  Scenario: Taking a note
    When I note
      """
      remember the milk
      """
    Then the note is "remember the milk"
