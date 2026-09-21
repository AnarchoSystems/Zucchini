Feature: Naming conventions

  Scenario: Generated names follow the stylesheet
    Given people exist
      | Name | Age | Color | Tag      | Nickname |
      | Ada  | 30  | red   | red;blue |          |
    Then there are 1 people
