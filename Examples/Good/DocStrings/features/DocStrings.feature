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
        """
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

    Scenario: Quoted YAML scalars retain their string type
      When I note this yaml
        """
        title: "false"
        priority: 0x10
        """
      Then the note is "false"
      And the priority is 16

    Scenario: Explicit YAML scalar tags are respected
      When I note this yaml
        """
        title: !!str 42
        priority: !!int 0o10
        """
      Then the note is "42"
      And the priority is 8

    Scenario: YAML anchors and merge keys are resolved
      When I note this yaml
        """
        defaults: &defaults {title: inherited title, priority: 4}
        <<: *defaults
        priority: 7
        """
      Then the note is "inherited title"
      And the priority is 7

    Scenario: YAML block scalars retain their contents
      When I note this yaml
        """
        title: |-
          first line
          second line
        priority: 2
        """
      Then the note is "first line\nsecond line"
      And the priority is 2
