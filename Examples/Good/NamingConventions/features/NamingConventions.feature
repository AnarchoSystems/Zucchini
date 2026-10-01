Feature: Naming conventions

  Scenario: Generated names follow the stylesheet
    Given people exist
      | Name | Age | Color | Tag      | Nickname | Origin  |
      | Ada  | 30  | red   | red;blue |          | fixture |
    Then there are 1 people
