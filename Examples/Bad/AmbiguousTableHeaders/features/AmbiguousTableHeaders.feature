Feature: Ambiguous table headers
  A table cell must belong to exactly one header, so rules can name the fact it
  states.

  Scenario: Two aliases of one field are rejected
    When I add these entries
      | value | amount | label |
      | 4     | 5      | both  |

  Scenario: A repeated header is rejected
    When I add these entries
      | value | label | label  |
      | 4     | first | second |
