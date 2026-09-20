Feature: Doc strings

  Rule: Plain text

    Scenario: A plain doc string arrives verbatim
      When I note
        """
        remember the milk
        """
      Then the note is "remember the milk"

    Scenario: Multiline text is preserved
      When I note
        """
        first line
        second line
        third line
        """
      Then the note is "first line\nsecond line\nthird line"

  Rule: Structured content

    Scenario: YAML is converted into a typed value
      When I note this yaml
        """yaml
        title: remember the milk
        priority: 4
        """
      Then the note is "remember the milk"
      And the priority is 4

    Scenario: JSON is converted into a typed value
      When I note this json
        """json
        {
          "title": "buy vegetables",
          "priority": 3
        }
        """
      Then the note is "buy vegetables"
      And the priority is 3

    Scenario: Missing priority uses its default
      When I note this yaml
        """
        title: no priority
        """
      Then the note is "no priority"
      And the priority is 0
