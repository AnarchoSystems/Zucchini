Feature: Naming conventions

  Scenario: Generated names follow the stylesheet
    Given people exist
      | name | age | color | tags     | nickname |
      | Ada  | 30  | red   | red;blue |          |
    Then there are 1 people
