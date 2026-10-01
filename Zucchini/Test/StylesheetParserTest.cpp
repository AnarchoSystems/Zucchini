#include "StylesheetParser.hpp"

#include <gtest/gtest.h>

namespace nZucchini {
TEST(Stylesheet, UsesDefaults) {
  Stylesheet stylesheet;
  Diagnostics errors;
  ASSERT_TRUE(parse_stylesheet("{}", stylesheet, errors)) << to_string(errors);
  EXPECT_FALSE(stylesheet.typeNaming.casing);
  EXPECT_FALSE(stylesheet.methodNaming.casing);
  EXPECT_FALSE(stylesheet.variableNaming.casing);
  EXPECT_FALSE(stylesheet.mapBaseNameIsSingular);
  EXPECT_EQ("Person",
            compose_type_name(stylesheet.typeNaming, "Person", false));
  EXPECT_EQ("Checkout", compose_class_name(stylesheet.typeNaming, "Checkout"));
  EXPECT_EQ("value",
            compose_variable_name(stylesheet.variableNaming, "value",
                                  VariableKind::Int, false, false, false));
}

TEST(Stylesheet, NamesFixtures)
{
  const auto yaml = R"(
cppConventions:
  types:
    blocks:
      - blockName: classPrefix
        onClass: c
    nameIt: [classPrefix, typeName]
)";
  Stylesheet stylesheet;
  Diagnostics errors;
  ASSERT_TRUE(parse_stylesheet(yaml, stylesheet, errors)) << to_string(errors);
  EXPECT_EQ("cCheckout",
            compose_class_name(stylesheet.typeNaming, "Checkout"));
}

TEST(Stylesheet, ParsesCommonIncludes) {
  const auto yaml = R"(
commonIncludes:
  - Common.h
  - Project/Common.h
)";
  Stylesheet stylesheet;
  Diagnostics errors;
  ASSERT_TRUE(parse_stylesheet(yaml, stylesheet, errors)) << to_string(errors);
  ASSERT_EQ(2u, stylesheet.commonIncludes.size());
  EXPECT_EQ("Common.h", stylesheet.commonIncludes[0]);
  EXPECT_EQ("Project/Common.h", stylesheet.commonIncludes[1]);
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
        onString: s
      - blockName: member
        onMember: m
        onNotMember: a
      - blockName: array
        onArray: l
        onNotArray: ""
      - blockName: map
        onMap: a
        onNotMap: ""
      - blockName: optional
        onOptional: o
        onMandatory: ""
      - blockName: mapKey
        onMapKeyType: kind
      - blockName: mapValue
        onMapValueType: kind
    nameIt: [member, array, map, optional, mapKey, mapValue, kind, variableName]
  methods:
    blocks:
      - blockName: methodKind
        onStepDefinition: s
        onHook: h
    nameIt: [f, methodKind, methodName]
)";
  Stylesheet stylesheet;
  Diagnostics errors;
  ASSERT_TRUE(parse_stylesheet(yaml, stylesheet, errors)) << to_string(errors);
  EXPECT_EQ("tPerson",
            compose_type_name(stylesheet.typeNaming, "Person", false));
  EXPECT_EQ("EColor", compose_type_name(stylesheet.typeNaming, "Color", true));
  EXPECT_EQ("fsDoThing",
            compose_method_name(stylesheet.methodNaming, "DoThing"));
  EXPECT_EQ("fhAroundStep",
            compose_method_name(stylesheet.methodNaming, "AroundStep", true));
  EXPECT_EQ("mleValue",
            compose_variable_name(stylesheet.variableNaming, "Value",
                                  VariableKind::Enum, true, true, false));
  EXPECT_EQ("aiCount",
            compose_variable_name(stylesheet.variableNaming, "Count",
                                  VariableKind::Int, false, false, false));
  EXPECT_EQ("massAdditionalProperties",
            compose_variable_name(stylesheet.variableNaming,
                                  "AdditionalProperties", VariableKind::String,
                                  true, false, false, VariableKind::String,
                                  VariableKind::String));
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

TEST(Stylesheet, RejectsUnknownMapTypeBlockReference) {
  const auto yaml = R"(
cppConventions:
  variables:
    blocks:
      - blockName: mapKey
        onMapKeyType: missingKind
    nameIt: [mapKey, variableName]
)";
  Stylesheet stylesheet;
  Diagnostics errors;
  EXPECT_FALSE(parse_stylesheet(yaml, stylesheet, errors));
  ASSERT_FALSE(errors.empty());
  EXPECT_NE(std::string::npos,
            errors.front().message.find("unknown naming block reference"));
}

TEST(Stylesheet, ParsesCasingOptions) {
  const auto yaml = R"(
cppConventions:
  types:
    casing: PascalCase
  methods:
    casing: camelCase
  variables:
    casing: snake_case
    mapBaseNameIsSingular: true
)";
  Stylesheet stylesheet;
  Diagnostics errors;
  ASSERT_TRUE(parse_stylesheet(yaml, stylesheet, errors)) << to_string(errors);
  ASSERT_TRUE(stylesheet.typeNaming.casing);
  EXPECT_EQ(Casing::PascalCase, *stylesheet.typeNaming.casing);
  ASSERT_TRUE(stylesheet.methodNaming.casing);
  EXPECT_EQ(Casing::CamelCase, *stylesheet.methodNaming.casing);
  ASSERT_TRUE(stylesheet.variableNaming.casing);
  EXPECT_EQ(Casing::SnakeCase, *stylesheet.variableNaming.casing);
  EXPECT_TRUE(stylesheet.mapBaseNameIsSingular);
}

TEST(Stylesheet, RejectsLegacyCasingSections) {
  Stylesheet stylesheet;
  Diagnostics errors;
  EXPECT_FALSE(parse_stylesheet("hooks:\n  aroundStep: camelCase\n",
                                stylesheet, errors));
  ASSERT_FALSE(errors.empty());
}

TEST(Stylesheet, RejectsLegacyCamelCaseSpelling) {
  Stylesheet stylesheet;
  Diagnostics errors;
  EXPECT_FALSE(parse_stylesheet(
      "cppConventions:\n  types:\n    casing: CamelCase\n", stylesheet,
      errors));
  ASSERT_FALSE(errors.empty());
}
} // namespace nZucchini
