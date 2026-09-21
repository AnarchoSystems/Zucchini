#include "MakeZucchini.hpp"

#include <cucumber/messages/pickle_doc_string.hpp>
#include <cucumber/messages/pickle_step.hpp>
#include <cucumber/messages/pickle_step_argument.hpp>
#include <cucumber/messages/pickle_table.hpp>
#include <cucumber/messages/pickle_table_cell.hpp>
#include <cucumber/messages/pickle_table_row.hpp>
#include <cucumber/messages/pickle_tag.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {
using namespace nZucchini;
namespace messages = cucumber::messages;

messages::pickle_step Step(std::string text) {
  messages::pickle_step step;
  step.id = "step-" + text;
  step.text = std::move(text);
  return step;
}

messages::pickle_step DocStringStep(std::string text, std::string content,
                                    std::string mediaType) {
  auto step = Step(std::move(text));

  messages::pickle_doc_string docString;
  docString.content = std::move(content);
  docString.media_type = std::move(mediaType);

  messages::pickle_step_argument argument;
  argument.doc_string = std::move(docString);
  step.argument = std::move(argument);

  return step;
}

messages::pickle_step
DataTableStep(std::string text,
              const std::vector<std::vector<std::string>> &rows) {
  auto step = Step(std::move(text));

  messages::pickle_table table;
  for (const auto &row : rows) {
    messages::pickle_table_row tableRow;
    for (const auto &value : row) {
      messages::pickle_table_cell cell;
      cell.value = value;
      tableRow.cells.push_back(std::move(cell));
    }
    table.rows.push_back(std::move(tableRow));
  }

  messages::pickle_step_argument argument;
  argument.data_table = std::move(table);
  step.argument = std::move(argument);

  return step;
}

messages::pickle Pickle(std::string name,
                        std::vector<messages::pickle_step> steps,
                        std::vector<std::string> tagNames = {}) {
  messages::pickle pickle;
  pickle.id = "pickle-" + name;
  pickle.uri = "in-memory.feature";
  pickle.name = std::move(name);
  pickle.language = "en";
  pickle.steps = std::move(steps);
  for (auto &tagName : tagNames) {
    messages::pickle_tag tag;
    tag.name = std::move(tagName);
    pickle.tags.push_back(std::move(tag));
  }
  return pickle;
}

struct LinkCase {
  LinkCase(std::string name, messages::pickle pickle, StepDefinitions manifest,
           std::string featureName, Zucchini expected)
      : name(std::move(name)), pickle(std::move(pickle)),
        manifest(std::move(manifest)), featureName(std::move(featureName)),
        expected(std::move(expected)) {}

  std::string name;
  messages::pickle pickle;
  StepDefinitions manifest;
  std::string featureName;
  Zucchini expected;

  friend std::ostream &operator<<(std::ostream &stream, const LinkCase &value) {
    return stream << value.name;
  }
};

struct LinkFailureCase {
  LinkFailureCase(std::string name, messages::pickle pickle,
                  StepDefinitions manifest,
                  std::vector<CodingPath> expectedPaths)
      : name(std::move(name)), pickle(std::move(pickle)),
        manifest(std::move(manifest)), expectedPaths(std::move(expectedPaths)) {
  }

  std::string name;
  messages::pickle pickle;
  StepDefinitions manifest;
  std::vector<CodingPath> expectedPaths;

  friend std::ostream &operator<<(std::ostream &stream,
                                  const LinkFailureCase &value) {
    return stream << value.name;
  }
};

template <typename TestCase>
std::string CaseName(const testing::TestParamInfo<TestCase> &info) {
  return info.param.name;
}

std::vector<LinkCase> LinkCases() {
  return {
      LinkCase(
          "PlainStep",
          Pickle("a scenario", {Step("I do nothing")}, {"@smoke", "@fast"}),
          StepDefinitions({StepDefinition("^I do nothing$", "doNothing")}),
          "My Feature",
          Zucchini(
              "a scenario", "My Feature",
              {ZucchiniStep("^I do nothing$", "doNothing", "I do nothing")}, {},
              {}, {"@smoke", "@fast"})),

      LinkCase(
          "TypedCaptures",
          Pickle("captures",
                 {Step(R"(I have 42 cukes named "Bob" which are true)")}),
          StepDefinitions({StepDefinition(
              R"RX(^I have (\d+) cukes named "([^"]*)" which are (true|false)$)RX",
              "haveCukes",
              {Argument("count", "int"), Argument("label", "string"),
               Argument("tasty", "bool")})}),
          "Captures",
          Zucchini(
              "captures", "Captures",
              {ZucchiniStep(
                  R"RX(^I have (\d+) cukes named "([^"]*)" which are (true|false)$)RX",
                  "haveCukes", R"(I have 42 cukes named "Bob" which are true)",
                  {Capture("count", 42), Capture("label", "Bob"),
                   Capture("tasty", true)})})),

      LinkCase(
          "DocString",
          Pickle("docstring",
                 {DocStringStep("I send a payload", R"({"a": 1})", "json")}),
          StepDefinitions(
              {StepDefinition("^I send a payload$", "sendPayload", {}, std::nullopt,
                       DocStringSpec("json", "Payload"))}),
          "DocStrings",
          Zucchini("docstring", "DocStrings",
                   {ZucchiniStep("^I send a payload$", "sendPayload",
                                 "I send a payload", {},
                                 DocStringArgument(R"({"a": 1})", "json"))})),

      LinkCase(
          "DataTableWithHeader",
          Pickle("table",
                 {DataTableStep(
                     "these people exist",
                     {{"name", "age"}, {"Alice", "30"}, {"Bob", "40"}})}),
          StepDefinitions(
              {StepDefinition("^these people exist$", "peopleExist", {},
                       DataTableSpec(TableDirection::Rows, true, "dynamic"))}),
          "Tables",
          Zucchini(
              "table", "Tables",
              {ZucchiniStep(
                  "^these people exist$", "peopleExist", "these people exist",
                  {},
                  DataTableArgument(
                      {nlohmann::json{{"name", "Alice"}, {"age", "30"}},
                       nlohmann::json{{"name", "Bob"}, {"age", "40"}}}))})),

      LinkCase(
          "DataTableWithoutHeader",
          Pickle("table", {DataTableStep("these values exist",
                                         {{"Alice", "30"}, {"Bob", "40"}})}),
          StepDefinitions(
              {StepDefinition("^these values exist$", "valuesExist", {},
                       DataTableSpec(TableDirection::Rows, false, "dynamic"))}),
          "Tables",
          Zucchini("table", "Tables",
                   {ZucchiniStep(
                       "^these values exist$", "valuesExist",
                       "these values exist", {},
                       DataTableArgument({nlohmann::json({"Alice", "30"}),
                                          nlohmann::json({"Bob", "40"})}))})),

      LinkCase(
          "ColumnOrientedDataTable",
          Pickle("table",
                 {DataTableStep("these people exist", {{"name", "Alice", "Bob"},
                                                       {"age", "30", "40"}})}),
          StepDefinitions({StepDefinition(
              "^these people exist$", "peopleExist", {},
              DataTableSpec(TableDirection::Columns, true, "dynamic"))}),
          "Tables",
          Zucchini(
              "table", "Tables",
              {ZucchiniStep(
                  "^these people exist$", "peopleExist", "these people exist",
                  {},
                  DataTableArgument(
                      {nlohmann::json{{"name", "Alice"}, {"age", "30"}},
                       nlohmann::json{{"name", "Bob"}, {"age", "40"}}}))})),
  };
}

std::vector<LinkFailureCase> LinkFailureCases() {
  return {
      LinkFailureCase("UnmatchedStep",
                      Pickle("unmatched", {Step("I do something unknown")}),
                      StepDefinitions({StepDefinition("^I do nothing$", "doNothing")}),
                      {std::string("steps[0]")}),

      LinkFailureCase(
          "UnexpectedDocString",
          Pickle("unexpected",
                 {DocStringStep("I do nothing", "payload", "text/plain")}),
          StepDefinitions({StepDefinition("^I do nothing$", "doNothing")}),
          {std::string("steps[0].docstring")}),

      LinkFailureCase(
          "MissingDataTable", Pickle("missing", {Step("these people exist")}),
          StepDefinitions(
              {StepDefinition("^these people exist$", "peopleExist", {},
                       DataTableSpec(TableDirection::Rows, true, "dynamic"))}),
          {std::string("steps[0].dataTable")}),

      LinkFailureCase(
          "MissingCaptureDeclaration",
          Pickle("capture", {Step("I have 42 cukes")}),
          StepDefinitions({StepDefinition("^I have (\\d+) cukes$", "haveCukes")}),
          {std::string("steps[0].arguments")}),

      LinkFailureCase("AmbiguousRegexCaptures",
                      Pickle("ambiguous", {Step("I do nothing")}),
                      StepDefinitions({StepDefinition("^I do (.*)$", "doSomething",
                                               {Argument("what", "string")}),
                                       StepDefinition("^I do nothing$", "doNothing")}),
                      {std::string("steps[0]")}),

      LinkFailureCase("DuplicateStepDefinitions",
                      Pickle("duplicate", {Step("I do nothing")}),
                      StepDefinitions({StepDefinition("^I do nothing$", "first"),
                                       StepDefinition("^I do nothing$", "second")}),
                      {std::string("steps[0]")}),

      LinkFailureCase("InvalidRegex",
                      Pickle("invalid regex", {Step("anything")}),
                      StepDefinitions({StepDefinition("^[$", "broken")}),
                      {std::string("steps[0].step")}),

      LinkFailureCase(
          "InvalidIntegerCapture",
          Pickle("invalid capture", {Step("I have nope cukes")}),
          StepDefinitions({StepDefinition("^I have (.*) cukes$", "haveCukes",
                                   {Argument("count", "int")})}),
          {std::string("steps[0].arguments[0]")}),

      LinkFailureCase(
          "InvalidEnumTableValue",
          Pickle("invalid enum",
                 {DataTableStep("tagged entries",
                                {{"value", "tags"}, {"2", "ultraviolet"}})}),
          StepDefinitions(
              {EnumType("Colour", "Colour_", {EnumCase("red", {"red"})}),
               StructType("TaggedEntry",
                          {StructField("value", "int"),
                           StructField("tags", "list", {}, false, std::nullopt,
                                       "Colour", ';')})},
              {StepDefinition(
                  "^tagged entries$", "taggedEntries", {},
                  DataTableSpec(TableDirection::Rows, true, "TaggedEntry"))}),
          {std::string("steps[0].dataTable[0].tags")}),

      LinkFailureCase(
          "MisspelledRequiredTableHeader",
          Pickle("header typo", {DataTableStep("entries", {{"vaule", "label"},
                                                           {"4", "typo"}})}),
          StepDefinitions(
              {StructType("Entry",
                          {StructField("value", "int", {"value", "amount"}),
                           StructField("label", "string", {}, true)})},
              {StepDefinition("^entries$", "entries", {},
                       DataTableSpec(TableDirection::Rows, true, "Entry"))}),
          {std::string("steps[0].dataTable[0].value"),
           std::string("steps[0].dataTable[0].vaule")}),

      LinkFailureCase(
          "MalformedTypedJsonDocString",
          Pickle("malformed doc string",
                 {DocStringStep("note", "{ nope", "json")}),
          StepDefinitions({StructType("Note", {StructField("title")})},
                          {StepDefinition("^note$", "note", {}, std::nullopt,
                                   DocStringSpec("json", "Note"))}),
          {std::string("steps[0].docstring")}),

      LinkFailureCase(
          "MissingTypedDocStringProperty",
          Pickle("missing doc string property",
                 {DocStringStep("note", R"({"priority": 2})", "json")}),
          StepDefinitions(
              {StructType("Note", {StructField("title"),
                                   StructField("priority", "int")})},
              {StepDefinition("^note$", "note", {}, std::nullopt,
                       DocStringSpec("json", "Note"))}),
          {std::string("steps[0].docstring.title")}),

      LinkFailureCase(
          "WrongTypedDocStringPropertyType",
          Pickle("wrong doc string property",
                 {DocStringStep("note", R"({"title": 42})", "json")}),
          StepDefinitions({StructType("Note", {StructField("title")})},
                          {StepDefinition("^note$", "note", {}, std::nullopt,
                                   DocStringSpec("json", "Note"))}),
          {std::string("steps[0].docstring.title")}),
  };
}

class Linking : public testing::TestWithParam<LinkCase> {};

TEST_P(Linking, YieldsExpectedZucchini) {
  Zucchini actual;
  Diagnostics errors;

  ASSERT_TRUE(make_zucchini(GetParam().pickle, GetParam().manifest,
                            GetParam().featureName, actual, errors))
      << to_string(errors);
  EXPECT_EQ(GetParam().expected, actual);
}

INSTANTIATE_TEST_SUITE_P(Pickles, Linking, testing::ValuesIn(LinkCases()),
                         CaseName<LinkCase>);

class LinkingFailures : public testing::TestWithParam<LinkFailureCase> {};

TEST_P(LinkingFailures, ReportsExpectedDiagnostics) {
  Zucchini zucchini;
  Diagnostics errors;

  EXPECT_FALSE(make_zucchini(GetParam().pickle, GetParam().manifest, "Feature",
                             zucchini, errors));
  EXPECT_EQ(GetParam().expectedPaths, paths_of(errors)) << to_string(errors);
}

INSTANTIATE_TEST_SUITE_P(Pickles, LinkingFailures,
                         testing::ValuesIn(LinkFailureCases()),
                         CaseName<LinkFailureCase>);
} // namespace
