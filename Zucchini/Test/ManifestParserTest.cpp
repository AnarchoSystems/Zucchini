#include "ManifestParser.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {
using namespace nZucchini;

struct ParseCase {
  ParseCase(std::string name, std::string yaml, StepDefinitions expected)
      : name(std::move(name)), yaml(std::move(yaml)),
        expected(std::move(expected)) {}

  std::string name;
  std::string yaml;
  StepDefinitions expected;

  friend std::ostream &operator<<(std::ostream &stream,
                                  const ParseCase &value) {
    return stream << value.name;
  }
};

struct ParseFailureCase {
  ParseFailureCase(std::string name, std::string yaml,
                   std::vector<CodingPath> expectedPaths,
                   bool expectsPosition = false)
      : name(std::move(name)), yaml(std::move(yaml)),
        expectedPaths(std::move(expectedPaths)),
        expectsPosition(expectsPosition) {}

  std::string name;
  std::string yaml;
  std::vector<CodingPath> expectedPaths;
  bool expectsPosition = false;

  friend std::ostream &operator<<(std::ostream &stream,
                                  const ParseFailureCase &value) {
    return stream << value.name;
  }
};

template <typename TestCase>
std::string CaseName(const testing::TestParamInfo<TestCase> &info) {
  return info.param.name;
}

std::vector<ParseCase> ParseCases() {
  return {
      ParseCase("Minimal",
                R"YAML(
steps:
  - step: ^I do nothing$
    methodName: doNothing
)YAML",
                StepDefinitions({StepDefinition("^I do nothing$", "doNothing")})),

      ParseCase(
          "TypedArguments",
          R"YAML(
steps:
  - step: ^I have (\d+) cukes named "([^"]*)" which are (true|false)$
    methodName: haveCukes
    arguments:
      - name: count
        type: int
      - name: label
        type: string
      - name: tasty
        type: bool
)YAML",
          StepDefinitions({StepDefinition(
              R"RX(^I have (\d+) cukes named "([^"]*)" which are (true|false)$)RX",
              "haveCukes",
              {Argument("count", "int"), Argument("label", "string"),
               Argument("tasty", "bool")})})),

      ParseCase("DataTableDefaults",
                R"YAML(
steps:
  - step: ^the table$
    methodName: table
    dataTable:
      header: true
)YAML",
                StepDefinitions(
                    {StepDefinition("^the table$", "table", {},
                             DataTableSpec(TableDirection::Rows, true))})),

      ParseCase(
          "DataTableDirectionsAndTypes",
          R"YAML(
steps:
  - step: ^columns$
    methodName: columns
    dataTable:
      direction: columns
      header: true
      type: Person
  - step: ^dynamic rows$
    methodName: dynamicRows
    dataTable:
      direction: rows
      type: dynamic
)YAML",
          StepDefinitions(
              {StepDefinition("^columns$", "columns", {},
                       DataTableSpec(TableDirection::Columns, true, "Person")),
               StepDefinition("^dynamic rows$", "dynamicRows", {},
                       DataTableSpec(TableDirection::Rows, true, "dynamic"))})),

      ParseCase("DocStringVariants",
                R"YAML(
steps:
  - step: ^null docstring$
    methodName: nullDocstring
    docstring: ~
  - step: ^flag docstring$
    methodName: flagDocstring
    docstring: true
  - step: ^typed docstring$
    methodName: typedDocstring
    docstring:
      contentType: json
      type: Person
  - step: ^no docstring$
    methodName: noDocstring
)YAML",
                StepDefinitions(
                    {StepDefinition("^null docstring$", "nullDocstring", {},
                             std::nullopt, DocStringSpec(std::nullopt)),
                     StepDefinition("^flag docstring$", "flagDocstring", {},
                             std::nullopt, DocStringSpec(std::nullopt)),
                     StepDefinition("^typed docstring$", "typedDocstring", {},
                             std::nullopt, DocStringSpec("json", "Person")),
                     StepDefinition("^no docstring$", "noDocstring")})),

      ParseCase(
          "EnumTypes",
          R"YAML(
types:
  - name: Colour
    kind: enum
    prefix: Colour_
    cases:
      - red
      - name: green
        values:
          - green
          - verde
  - name: Imported
    kind: enum
    prefix: ""
    cases:
      - one
    imported: true
    verbatimType: ns::Imported
steps:
  - step: ^nothing$
    methodName: nothing
)YAML",
          StepDefinitions({EnumType("Colour", "Colour_",
                                    {EnumCase("red", {"red"}),
                                     EnumCase("green", {"green", "verde"})}),
                           EnumType("Imported", "", {EnumCase("one", {"one"})},
                                    true, "ns::Imported")},
                          {StepDefinition("^nothing$", "nothing")})),

      ParseCase(
          "EnumPrefixDefaultsToEmpty",
          R"YAML(
types:
  - name: Colour
    kind: enum
    cases:
      - red
steps:
  - step: ^nothing$
    methodName: nothing
)YAML",
          StepDefinitions({EnumType("Colour", "", {EnumCase("red", {"red"})})},
                          {StepDefinition("^nothing$", "nothing")})),

      ParseCase("SingleHeaderAsPlainString",
                R"YAML(
types:
  - name: Person
    kind: struct
    fields:
      - name: firstName
        header: first_name
steps:
  - step: ^nothing$
    methodName: nothing
)YAML",
                StepDefinitions(
                    {StructType("Person", {StructField("firstName", "string",
                                                       {"first_name"})})},
                    {StepDefinition("^nothing$", "nothing")})),

      ParseCase("ImportedStructWithParserDescription",
                R"YAML(
types:
  - name: Person
    kind: struct
    imported: true
    verbatimType: external::Person
    fields:
      - name: firstName
        header:
          - first_name
          - "First Name"
      - name: age
        type: int
steps:
  - step: ^nothing$
    methodName: nothing
)YAML",
                StepDefinitions(
                    {StructType("Person",
                                {StructField("firstName", "string",
                                             {"first_name", "First Name"}),
                                 StructField("age", "int")},
                                false, true, "external::Person")},
                    {StepDefinition("^nothing$", "nothing")})),

      ParseCase("StructTypes",
                R"YAML(
types:
  - name: Person
    kind: struct
    additionalProperties: true
    fields:
      - name: firstName
        header:
          - "First Name"
          - first_name
      - name: age
        type: int
        default: 42
      - name: nickname
        optional: true
      - name: hobbies
        type: list
        content: string
        separator: ";"
steps:
  - step: ^nothing$
    methodName: nothing
)YAML",
                StepDefinitions(
                    {StructType("Person",
                                {StructField("firstName", "string",
                                             {"First Name", "first_name"}),
                                 StructField("age", "int", {}, false,
                                             nlohmann::json(42)),
                                 StructField("nickname", "string", {}, true),
                                 StructField("hobbies", "list", {}, false,
                                             std::nullopt, "string", ';')},
                                true)},
                    {StepDefinition("^nothing$", "nothing")})),
  };
}

std::vector<ParseFailureCase> ParseFailureCases() {
  return {
      ParseFailureCase("MissingMethodName",
                       R"YAML(
steps:
  - step: ^I do nothing$
)YAML",
                       {std::string("steps[0]")}),

      ParseFailureCase("MissingSteps",
                       R"YAML(
types: []
)YAML",
                       {std::string()}),

      ParseFailureCase("StepsOfWrongNodeType",
                       R"YAML(
steps: nope
)YAML",
                       {std::string("steps")}),

      ParseFailureCase("UnknownKey",
                       R"YAML(
steps:
  - step: ^I do nothing$
    methodName: doNothing
    unexpected: true
)YAML",
                       {std::string("steps[0]")}),

      ParseFailureCase("UnanchoredStepRegex",
                       R"YAML(
steps:
  - step: I do nothing
    methodName: doNothing
)YAML",
                       {std::string("steps[0].step")}),

      ParseFailureCase("InvalidMethodName",
                       R"YAML(
steps:
  - step: ^I do nothing$
    methodName: does not compile
)YAML",
                       {std::string("steps[0].methodName")}),

      ParseFailureCase("CppKeywordIdentifier",
                       R"YAML(
steps:
  - step: ^I do nothing$
    methodName: class
)YAML",
                       {std::string("steps[0].methodName")}),

      ParseFailureCase("ImportedStructRequiresFieldDescription",
                       R"YAML(
types:
  - name: Person
    kind: struct
    imported: true
steps:
  - step: ^nothing$
    methodName: nothing
)YAML",
                       {std::string("types[0]")}),

      ParseFailureCase("MalformedYaml",
                       R"YAML(
steps:
  - step: ^ok$
   methodName: broken
)YAML",
                       {std::string()}, true),
  };
}

class ManifestParsing : public testing::TestWithParam<ParseCase> {};

TEST_P(ManifestParsing, YieldsExpectedManifest) {
  StepDefinitions actual;
  Diagnostics errors;

  ASSERT_TRUE(parse_step_def_manifest(GetParam().yaml, actual, errors))
      << to_string(errors);
  EXPECT_EQ(GetParam().expected, actual);
}

INSTANTIATE_TEST_SUITE_P(Manifests, ManifestParsing,
                         testing::ValuesIn(ParseCases()), CaseName<ParseCase>);

class ManifestParsingFailures
    : public testing::TestWithParam<ParseFailureCase> {};

TEST_P(ManifestParsingFailures, ReportsExpectedDiagnostics) {
  StepDefinitions manifest;
  Diagnostics errors;

  EXPECT_FALSE(parse_step_def_manifest(GetParam().yaml, manifest, errors));
  EXPECT_EQ(GetParam().expectedPaths, paths_of(errors)) << to_string(errors);

  if (GetParam().expectsPosition) {
    ASSERT_FALSE(errors.empty());
    EXPECT_TRUE(errors.front().line.has_value()) << to_string(errors);
    EXPECT_TRUE(errors.front().column.has_value()) << to_string(errors);
  }
}

INSTANTIATE_TEST_SUITE_P(Manifests, ManifestParsingFailures,
                         testing::ValuesIn(ParseFailureCases()),
                         CaseName<ParseFailureCase>);

TEST(Manifest, FindsDeclaredTypesByName) {
  const StepDefinitions manifest(
      {StructType("Person", {StructField("firstName")})},
      {StepDefinition("^nothing$", "nothing")});

  const auto *person = find_type(manifest, "Person");
  ASSERT_NE(nullptr, person);
  EXPECT_EQ("Person", type_name(*person));
  EXPECT_EQ(nullptr, find_type(manifest, "Nobody"));
}
} // namespace
