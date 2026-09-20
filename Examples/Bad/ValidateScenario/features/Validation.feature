Feature: Scenario validation

  Scenario: A note requires setup
    When I note
      """
      this should be rejected during discovery
      """