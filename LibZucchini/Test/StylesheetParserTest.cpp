#include "Zucchini/StylesheetParser.hpp"

#include <gtest/gtest.h>

namespace nZucchini {
TEST(Stylesheet, UsesDefaults) {
  Stylesheet stylesheet;
  Diagnostics errors;
  ASSERT_TRUE(parse_stylesheet("{}", stylesheet, errors)) << to_string(errors);
  EXPECT_EQ(Casing::SnakeCase, stylesheet.aroundStepCasing);
  EXPECT_EQ(Casing::PascalCase, stylesheet.snippetClassCasing);
  EXPECT_EQ("Person",
            compose_type_name(stylesheet.typeNaming, "Person", false));
  EXPECT_EQ("value",
            compose_variable_name(stylesheet.variableNaming, "value",
                                  VariableKind::Int, false, false, false));
}

TEST(Stylesheet, ComposesIndependentVariableBlocks) {
  const auto yaml = R"(
cppConventions:
  types:
    blocks:
      - blockName: prefix
        onEnum: E
        onStruct: t
    nameIt: [prefix, typeName]
  variables:
    blocks:
      - blockName: kind
        onEnum: e
        onInt: i
      - blockName: member
        onMember: m
        onNotMember: a
      - blockName: list
        onArray: l
        onNotArray: ""
      - blockName: optional
        onOptional: o
        onMandatory: ""
    nameIt: [member, list, optional, kind, variableName]
  methods:
    nameIt: [f, methodName]
)";
  Stylesheet stylesheet;
  Diagnostics errors;
  ASSERT_TRUE(parse_stylesheet(yaml, stylesheet, errors)) << to_string(errors);
  EXPECT_EQ("tPerson",
            compose_type_name(stylesheet.typeNaming, "Person", false));
  EXPECT_EQ("EColor", compose_type_name(stylesheet.typeNaming, "Color", true));
  EXPECT_EQ("fDoThing",
            compose_method_name(stylesheet.methodNaming, "DoThing"));
  EXPECT_EQ("mleValue",
            compose_variable_name(stylesheet.variableNaming, "Value",
                                  VariableKind::Enum, true, true, false));
  EXPECT_EQ("aiCount",
            compose_variable_name(stylesheet.variableNaming, "Count",
                                  VariableKind::Int, false, false, false));
}

TEST(Stylesheet, RejectsMixedVariableConditionGroups) {
  const auto yaml = R"(
cppConventions:
  variables:
    blocks:
      - blockName: invalid
        onMember: m
        onEnum: e
)";
  Stylesheet stylesheet;
  Diagnostics errors;
  EXPECT_FALSE(parse_stylesheet(yaml, stylesheet, errors));
  ASSERT_FALSE(errors.empty());
  EXPECT_EQ("cppConventions.variables.blocks[0]", errors.front().path);
}

TEST(Stylesheet, ParsesCasingOptions) {
  const auto yaml = R"(
hooks:
  aroundStep: camelCase
  validateScenario: CamelCase
snippets:
  methods: CamelCase
  classes: snake_case
)";
  Stylesheet stylesheet;
  Diagnostics errors;
  ASSERT_TRUE(parse_stylesheet(yaml, stylesheet, errors)) << to_string(errors);
  EXPECT_EQ(Casing::CamelCase, stylesheet.aroundStepCasing);
  EXPECT_EQ(Casing::PascalCase, stylesheet.validateScenarioCasing);
  EXPECT_EQ(Casing::PascalCase, stylesheet.snippetMethodCasing);
  EXPECT_EQ(Casing::SnakeCase, stylesheet.snippetClassCasing);
}
} // namespace nZucchini
