#include "Lowering.hpp"
#include "StylesheetParser.hpp"

#include <gtest/gtest.h>

namespace nZucchini {
TEST(Lowering, AppliesEnumPrefixToImportedCases) {
  const StepDefinitions manifest(
      {EnumType("QualityClass", "qc_", {EnumCase("IO", {"IO"})}, true,
                 "EQualityClass")},
      {StepDefinition("^nothing$", "nothing")});
  Stylesheet stylesheet;

  const auto fixture = lower(manifest, stylesheet, "Fixture");

  ASSERT_EQ(1u, fixture.enums.size());
  ASSERT_EQ(1u, fixture.enums.front().cases.size());
  EXPECT_EQ("EQualityClass::qc_IO", fixture.enums.front().cases.front().cppName);
}

TEST(Lowering, GeneratesArgsTypesOnlyForArgumentSteps) {
  const StepDefinitions manifest(
      {StepDefinition("^I add (.*)$", "addEntry",
                      {Argument("Count", "int")}),
       StepDefinition("^I reset$", "reset")});

  Stylesheet defaults;
  const auto defaultFixture = lower(manifest, defaults, "ArgsFixture");
  ASSERT_EQ(2u, defaultFixture.steps.size());
  EXPECT_FALSE(defaultFixture.hasTags);
  EXPECT_TRUE(defaultFixture.tags.empty());
  EXPECT_TRUE(defaultFixture.steps[0].hasArgs);
  EXPECT_EQ("addEntryArgs", defaultFixture.steps[0].argsTypeName);
  ASSERT_EQ(1u, defaultFixture.steps[0].arguments.size());
  EXPECT_EQ("long", defaultFixture.steps[0].arguments[0].valueType);
  EXPECT_NE(std::string::npos,
            defaultFixture.steps[0].arguments[0].decoder.find("captures.at(0)"));
  EXPECT_FALSE(defaultFixture.steps[1].hasArgs);
  EXPECT_TRUE(defaultFixture.steps[1].argsTypeName.empty());

  Stylesheet custom;
  Diagnostics errors;
  ASSERT_TRUE(parse_stylesheet(
      R"(
cppConventions:
  types:
    casing: PascalCase
    blocks:
      - blockName: argsPrefix
        onArgsType: operation_
    nameIt: [argsPrefix, typeName]
)",
      custom, errors))
      << to_string(errors);
  const auto customFixture = lower(manifest, custom, "ArgsFixture");
  EXPECT_EQ("operation_addEntryArgs", customFixture.steps[0].argsTypeName);
  EXPECT_TRUE(customFixture.steps[1].argsTypeName.empty());
}

TEST(Lowering, PreservesUserNameCasingWhileApplyingNamingBlocks) {
    const ParsedManifest manifest{
      StepDefinitions(
        {EnumType("userRole", "", {EnumCase("admin", {"admin"})}),
         StructType("userRecord", {StructField("displayName")})},
        {StepDefinition("^user (.*)$", "createUser",
                {Argument("userName", "string")})}),
      {{"IntroducesUser"}}};
  Stylesheet stylesheet;
  Diagnostics errors;
  ASSERT_TRUE(parse_stylesheet(
      R"(
cppConventions:
  types:
    casing: PascalCase
    blocks:
      - blockName: classPrefix
        onClass: class_
      - blockName: enumPrefix
        onEnum: enum_
      - blockName: structPrefix
        onStruct: struct_
      - blockName: argsPrefix
        onArgsType: args_
    nameIt: [classPrefix, enumPrefix, structPrefix, argsPrefix, typeName]
  methods:
    casing: PascalCase
    blocks:
      - blockName: methodKind
        onStepDefinition: step_
        onHook: hook_
    nameIt: [methodKind, methodName]
  variables:
    casing: snake_case
    blocks:
      - blockName: member
        onMember: m_
        onNotMember: a_
    nameIt: [member, variableName]
)",
      stylesheet, errors))
      << to_string(errors);

  const auto fixture = lower(manifest, stylesheet, "myFixture");

  EXPECT_EQ("class_myFixture", fixture.name);
  EXPECT_EQ("class_ImyFixture", fixture.interfaceName);
  ASSERT_EQ(1u, fixture.enums.size());
  EXPECT_EQ("enum_userRole", fixture.enums.front().cppName);
  ASSERT_EQ(1u, fixture.structs.size());
  EXPECT_EQ("struct_userRecord", fixture.structs.front().cppName);
  EXPECT_EQ("m_displayName", fixture.structs.front().fields.front().cppName);
  ASSERT_EQ(1u, fixture.steps.size());
  EXPECT_EQ("step_createUser", fixture.steps.front().methodName);
  EXPECT_EQ("args_createUserArgs", fixture.steps.front().argsTypeName);
  EXPECT_EQ("a_userName", fixture.steps.front().arguments.front().name);
  EXPECT_EQ("enum_IntroducesUserSteps", fixture.tags.front().stepsTypeName);
  EXPECT_EQ("hook_AroundStep", fixture.aroundStepName);
}

TEST(Lowering, GeneratesTagDispatchMetadataUsingTypeNaming) {
    const ParsedManifest manifest{
      StepDefinitions({StepDefinition("^users are introduced$",
                      "introducesUsers"),
               StepDefinition("^a user is referenced$",
                      "referencesUser")}),
      {{"IntroducesUser"}, {"IntroducesUser", "ReferencesUser"}}};
  Stylesheet stylesheet;
  Diagnostics errors;
  ASSERT_TRUE(parse_stylesheet(
      "cppConventions:\n  types:\n    casing: snake_case\n", stylesheet,
      errors))
      << to_string(errors);

  const auto fixture = lower(manifest, stylesheet, "TaggedFixture");

  EXPECT_TRUE(fixture.hasTags);
  EXPECT_EQ("step_tag", fixture.stepTagName);
  EXPECT_EQ("step_tags", fixture.stepTagsName);
  EXPECT_EQ("2", fixture.tagCount);
  ASSERT_EQ(2u, fixture.tags.size());
  EXPECT_EQ("IntroducesUserSteps", fixture.tags[0].stepsTypeName);
  ASSERT_EQ(2u, fixture.tags[0].methods.size());
  EXPECT_TRUE(fixture.tags[0].methods[0].tagged);
  EXPECT_TRUE(fixture.tags[0].methods[1].tagged);
  EXPECT_EQ("ReferencesUserSteps", fixture.tags[1].stepsTypeName);
  EXPECT_FALSE(fixture.tags[1].methods[0].tagged);
  EXPECT_TRUE(fixture.tags[1].methods[1].tagged);
}

TEST(Lowering, RejectsTagStepsNameCollidingWithManifestType) {
  const ParsedManifest manifest{
      StepDefinitions({StructType("ExistingSteps", {StructField("value")})},
                      {StepDefinition("^a tagged step$", "taggedStep")}),
      {{"Existing"}}};
  Stylesheet stylesheet;

  try {
    (void)lower(manifest, stylesheet, "TypeCollision");
    FAIL() << "expected tag Steps name to collide with manifest struct";
  } catch (const std::runtime_error &failure) {
    EXPECT_STREQ(
        "tag label 'Existing' generates step enum 'ExistingSteps' which "
        "collides with manifest struct 'ExistingSteps'",
        failure.what());
  }
}

TEST(Lowering, RejectsGeneratedTypeNamesThatCollideAfterNameIt) {
    const ParsedManifest manifest{
      StepDefinitions({StepDefinition("^first tagged step$", "firstTaggedStep"),
               StepDefinition("^second tagged step$",
                      "secondTaggedStep")}),
      {{"FooBar"}, {"foo_bar"}}};
  Stylesheet stylesheet;
  Diagnostics errors;
  ASSERT_TRUE(parse_stylesheet(
      R"(
cppConventions:
  types:
    blocks:
      - blockName: fixedName
        onEnum: fixed_
    nameIt: [fixedName]
)", stylesheet, errors))
      << to_string(errors);

  try {
    (void)lower(manifest, stylesheet, "TagCollision");
    FAIL() << "expected normalized tag Steps-name collision to be rejected";
  } catch (const std::runtime_error &failure) {
    EXPECT_STREQ(
      "C++ type 'fixed_' for the generated StepTag enum collides with the "
      "generated StepMethod enum",
      failure.what());
  }
}

TEST(Lowering, RejectsDuplicateArgsFieldNamesAfterVariableNaming) {
  const StepDefinitions manifest(
      {StepDefinition("^I add (.*) and (.*)$", "addEntry",
                      {Argument("Count", "int"), Argument("count", "int")})});
  Stylesheet stylesheet;
  Diagnostics errors;
  ASSERT_TRUE(parse_stylesheet(
      R"(
cppConventions:
  variables:
    blocks:
      - blockName: fixedArgumentName
        onNotMember: argument
    nameIt: [fixedArgumentName]
)", stylesheet, errors))
      << to_string(errors);
  EXPECT_THROW(lower(manifest, stylesheet, "ArgsCollision"),
               std::runtime_error);
}

TEST(Lowering, NamesHooksAndSingularMapMembers) {
  const StepDefinitions manifest(
      {StructType("Entry", {StructField("value")}, true)},
      {StepDefinition("^an entry$", "addEntry")});
  Stylesheet stylesheet;
  Diagnostics errors;
  ASSERT_TRUE(parse_stylesheet(
      R"(
cppConventions:
  methods:
    casing: PascalCase
    blocks:
      - blockName: methodKind
        onStepDefinition: step
        onHook: hook
    nameIt: [methodKind, methodName]
  variables:
    casing: camelCase
    mapBaseNameIsSingular: true
    blocks:
      - blockName: member
        onMember: m
      - blockName: map
        onMap: a
        onNotMap: ""
      - blockName: kindPrefix
        onString: s
      - blockName: key
        onMapKeyType: kindPrefix
      - blockName: value
        onMapValueType: kindPrefix
    nameIt: [member, map, key, value, variableName]
)",
      stylesheet, errors))
      << to_string(errors);

  const auto fixture = lower(manifest, stylesheet, "entryFixture");

  ASSERT_EQ(1u, fixture.steps.size());
  EXPECT_EQ("stepaddEntry", fixture.steps.front().methodName);
  EXPECT_EQ("hookAroundStep", fixture.aroundStepName);
  EXPECT_EQ("hookValidateScenario", fixture.validateScenarioName);
  ASSERT_EQ(1u, fixture.structs.size());
  EXPECT_EQ("massadditionalProperty",
            fixture.structs.front().additionalPropertiesName);
  const auto manifestJson = nlohmann::json::parse(
      nlohmann::json::parse(fixture.stepDefinitionsJson).get<std::string>());
  EXPECT_EQ(fixture.steps.front().methodName,
            manifestJson.at("steps").at(0).at("methodName"));

  Stylesheet pluralStylesheet;
  ASSERT_TRUE(parse_stylesheet(
      R"(
cppConventions:
  variables:
    casing: PascalCase
    blocks:
      - blockName: member
        onMember: m
      - blockName: map
        onMap: a
        onNotMap: ""
      - blockName: kindPrefix
        onString: s
      - blockName: key
        onMapKeyType: kindPrefix
      - blockName: value
        onMapValueType: kindPrefix
    nameIt: [member, map, key, value, variableName]
)",
      pluralStylesheet, errors))
      << to_string(errors);
  const auto pluralFixture = lower(manifest, pluralStylesheet, "EntryFixture");
  ASSERT_EQ(1u, pluralFixture.structs.size());
  EXPECT_EQ("massAdditionalProperties",
            pluralFixture.structs.front().additionalPropertiesName);

    const StepDefinitions importedManifest(
      {StructType("ImportedEntry", {StructField("value")}, true, true)},
      {StepDefinition("^an imported entry$", "addImportedEntry")});
    const auto importedFixture =
      lower(importedManifest, stylesheet, "ImportedFixture");
    EXPECT_EQ("additionalProperties",
        importedFixture.structs.front().additionalPropertiesName);
}
} // namespace nZucchini
