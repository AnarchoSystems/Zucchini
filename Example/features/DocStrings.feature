Feature: Doc strings

    Background:
        Given I start with 0

    Scenario: A YAML doc string is converted during discovery
        When I note this yaml
            """yaml
            title: remember the milk
            priority: 4
            """
        Then the result is 4
        And the note is "remember the milk"

    Scenario: A doc string without a media type is accepted
        When I note this yaml
            """
            title: no media type
            """
        Then the result is 0
        And the note is "no media type"

    Scenario: An untyped doc string arrives verbatim
        When I note
            """
            plain text
            """
        Then the note is "plain text"

    Scenario: A JSON doc string is parsed into a typed value
        When I note this json
            """json
            {
                "title": "remember the milk",
                "priority": 3
            }
            """
        Then the result is 3
        And the note is "remember the milk"
