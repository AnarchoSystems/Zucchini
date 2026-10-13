Feature: Scalar list elements

  Scenario: Builtin scalars and enums in named tables
    Then these scalar lists are decoded
      | ints  | integers | longs | floats  | doubles | bools   | booleans | strings     | colours   | optionalInts | optionalStrings | defaultedInts |
      | 2, -3 | 2, -3    | 2, -3 | 2.5, -3 | 2.5, -3 | yes, no | TRUE, 0  | alpha, beta | red, blue |              |                 |               |

  Scenario: Optional list columns may be missing
    Then these scalar lists are decoded
      | ints  | integers | longs | floats  | doubles | bools   | booleans | strings     | colours   |
      | 2, -3 | 2, -3    | 2, -3 | 2.5, -3 | 2.5, -3 | yes, no | TRUE, 0  | alpha, beta | red, blue |

  Scenario: Builtin scalars in column tables
    Then these scalar lists are decoded by column
      | ints     | 2, -3       |
      | integers | 2, -3       |
      | longs    | 2, -3       |
      | floats   | 2.5, -3     |
      | doubles  | 2.5, -3     |
      | bools    | yes, no     |
      | booleans | TRUE, 0     |
      | strings  | alpha, beta |
      | colours  | red, blue   |

  Scenario: Builtin scalars in positional tables
    Then these scalar lists are decoded by position
      | 2, -3 | 2, -3 | 2, -3 | 2.5, -3 | 2.5, -3 | yes, no | TRUE, 0 | alpha, beta | red, blue | | |

  Scenario: Builtin scalar lists from JSON
    Then these scalar lists are decoded from json
      """
      {"ints":[2,-3],"integers":[2,-3],"longs":[2,-3],"floats":[2.5,-3],"doubles":[2.5,-3],"bools":[true,false],"booleans":[true,false],"strings":["alpha","beta"],"colours":["red","blue"],"optionalInts":null}
      """

  Scenario: Builtin scalar lists from YAML
    Then these scalar lists are decoded from yaml
      """yaml
      ints: [2, -3]
      integers: [2, -3]
      longs: [2, -3]
      floats: [2.5, -3]
      doubles: [2.5, -3]
      bools: [true, false]
      booleans: [true, false]
      strings: [alpha, beta]
      colours: [red, blue]
      """

  Scenario: Empty optional scalars are absent
    Then these optional scalars are absent
      | count | amount | enabled | label |
      |       |        |         |       |

  Scenario: Missing optional scalar headers are absent
    Then these optional scalars are absent
      | count |
      |       |

  Scenario: Zero and false optional scalars remain present
    Then these optional scalars are present
      | count | amount | enabled | label |
      | 0     | 0.0    | false   | text  |

  Scenario: Optional list values remain present
    Then these optional lists are present
      | ints  | integers | longs | floats  | doubles | bools   | booleans | strings     | colours   | optionalInts | optionalStrings |
      | 2, -3 | 2, -3    | 2, -3 | 2.5, -3 | 2.5, -3 | yes, no | TRUE, 0  | alpha, beta | red, blue | 0, -1        | one, two        |
