Feature: Invalid scalar list elements

  Scenario: Invalid elements in typed table lists are rejected
    When these lists are invalid
      | ints    | decimals | flags        |
      | 2, nope | 2.5, bad | true, maybe  |

  Scenario: Wrong element types in JSON lists are rejected
    When these json lists are invalid
      """json
      {"ints":[2,"nope"],"decimals":[2.5,false],"flags":[true,1]}
      """
