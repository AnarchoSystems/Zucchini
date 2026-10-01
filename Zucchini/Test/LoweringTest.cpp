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
  EXPECT_EQ("stepAddEntry", fixture.steps.front().methodName);
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
