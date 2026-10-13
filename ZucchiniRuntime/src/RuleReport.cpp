#include "Zucchini/Runtime/RuleReport.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <random>
#include <sstream>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace nZucchini {
namespace {
constexpr char dashboard[] =
#include "RuleDashboard.inc"
    ;

std::string unique_id() {
  std::ostringstream stream;
  stream.imbue(std::locale::classic());
  stream << std::hex
         << std::chrono::high_resolution_clock::now().time_since_epoch().count()
         << '-' << std::random_device{}() << '-' << std::random_device{}();
  return stream.str();
}

void atomic_write(const std::filesystem::path &path,
                  const std::string &content) {
  std::filesystem::create_directories(path.parent_path());
  const auto temporary = path.parent_path() / (".tmp-" + unique_id());
  try {
    std::ofstream stream;
    stream.exceptions(std::ios::failbit | std::ios::badbit);
    stream.open(temporary, std::ios::binary);
    stream << content;
    stream.close();
#ifdef _WIN32
    if (!MoveFileExW(temporary.c_str(), path.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
      throw std::system_error(GetLastError(), std::system_category(),
                              "cannot replace report file");
    }
#else
    std::filesystem::rename(temporary, path);
#endif
  } catch (...) {
    std::error_code ignored;
    std::filesystem::remove(temporary, ignored);
    throw;
  }
}

void source_location(nlohmann::json &location,
                     const std::string &featureDirectory,
                     const char *key = "uri") {
  if (featureDirectory.empty() || !location.contains(key) ||
      !location.at(key).is_string() ||
      location.at(key).get<std::string>().empty())
    return;
  std::error_code error;
  const auto root = std::filesystem::absolute(featureDirectory, error);
  if (error)
    return;
  const auto path =
      std::filesystem::absolute(location.at(key).get<std::string>(), error);
  if (error)
    return;
  const auto relative =
      path.lexically_normal().lexically_relative(root.lexically_normal());
  if (!relative.empty())
    location["feature_relative_path"] = relative.generic_string();
}

std::string xml_escape(const std::string &text) {
  std::string result;
  for (const unsigned char character : text) {
    switch (character) {
    case '&':
      result += "&amp;";
      break;
    case '<':
      result += "&lt;";
      break;
    case '>':
      result += "&gt;";
      break;
    case '\"':
      result += "&quot;";
      break;
    case '\'':
      result += "&apos;";
      break;
    default:
      // XML 1.0 excludes these control characters, which can occur in test
      // output.
      if (character >= 32 || character == '\n' || character == '\r' ||
          character == '\t') {
        result += static_cast<char>(character);
      }
    }
  }
  return result;
}

std::string junit(const nlohmann::json &result) {
  const auto status = result.at("status").get<std::string>();
  const auto fixture = xml_escape(result.at("fixture").get<std::string>());
  const auto name = xml_escape(result.at("test_name").get<std::string>());
  std::ostringstream xml;
  xml.imbue(std::locale::classic());
  xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      << "<testsuites><testsuite name=\"" << fixture
      << "\" tests=\"1\" failures=\"" << (status == "failed" ? 1 : 0)
      << "\" skipped=\"" << (status == "skipped" ? 1 : 0)
      << "\"><testcase classname=\"" << fixture << "\" name=\"" << name
      << "\" time=\"" << result.value("duration_ms", 0.0) / 1000.0 << "\">";
  if (status == "failed" || status == "skipped") {
    const auto element = status == "failed" ? "failure" : "skipped";
    xml << '<' << element << '>';
    for (const auto &part : result.at("messages")) {
      if (part.contains("gherkin_source")) {
        xml << "Gherkin: "
            << xml_escape(part.at("gherkin_source").get<std::string>()) << '\n';
      }
      xml << xml_escape(part.value("file", std::string())) << ':'
          << part.value("line", 0) << ": "
          << xml_escape(part.at("message").get<std::string>()) << '\n';
    }
    xml << "</" << element << '>';
  }
  xml << "</testcase></testsuite></testsuites>\n";
  return xml.str();
}
} // namespace

std::string rule_result_key(const std::string &fixture,
                            const std::string &testName) {
  // Stable, compact file names also avoid Windows path-length and
  // invalid-character problems.
  std::uint64_t hash = 14695981039346656037ULL;
  const auto identity = fixture + '\0' + testName;
  for (const unsigned char character : identity) {
    hash = (hash ^ character) * 1099511628211ULL;
  }
  std::ostringstream stream;
  stream.imbue(std::locale::classic());
  stream << std::hex << std::setfill('0') << std::setw(16) << hash;
  return stream.str();
}

std::string rule_report_build_id(const std::string &executable) {
  const std::filesystem::path path(executable);
  // The normal CMake and Test Explorer launch paths identify the executable.
  if (!std::filesystem::exists(path)) {
    return {};
  }
  return std::to_string(std::filesystem::file_size(path)) + ":" +
         std::to_string(
             std::filesystem::last_write_time(path).time_since_epoch().count());
}

void store_rule_report(const std::string &directory, nlohmann::json report) {
  const std::filesystem::path root(directory);
  if (std::filesystem::exists(root / "rules.json")) {
    std::ifstream previous(root / "rules.json");
    const auto existing = nlohmann::json::parse(previous);
    if (existing.at("fixture") != report.at("fixture")) {
      throw std::invalid_argument(
          "Each fixture needs its own rule report directory");
    }
  }
  // Snapshot each feature once: published reports remain useful without the
  // checkout.
  report["sources"] = nlohmann::json::object();
  const auto fixture = report.at("fixture").get<std::string>();
  for (auto &scenario : report.at("scenarios")) {
    source_location(scenario, report.value("feature_directory", std::string()));
    const auto uri = scenario.value("uri", std::string());
    if (!uri.empty() && !report["sources"].contains(uri)) {
      std::ifstream source(uri, std::ios::binary);
      if (source) {
        auto lines = nlohmann::json::array();
        std::string line;
        while (std::getline(source, line)) {
          if (!line.empty() && line.back() == '\r')
            line.pop_back();
          lines.push_back(line);
        }
        report["sources"][uri] = std::move(lines);
      }
    }
    const auto name = scenario.at("test_name").get<std::string>();
    const auto key = rule_result_key(fixture, name);
    scenario["result_key"] = key;
    if (!std::filesystem::exists(root / "results" / (key + ".js"))) {
      store_rule_test_result(directory,
                             {{"fixture", fixture},
                              {"test_name", name},
                              {"status", "not_run"},
                              {"discovery_id", ""},
                              {"messages", nlohmann::json::array()}});
    }
  }
  for (auto &rule : report.at("rules")) {
    for (auto &event : rule.at("events")) {
      source_location(event, report.value("feature_directory", std::string()));
    }
  }
  // Discovery refreshes with identical inputs preserve execution evidence. The
  // generated executable includes its build identity and full plan
  // fingerprints.
  report.erase("discovery_id");
  report.erase("discovered_at_ms");
  report["discovery_id"] = rule_result_key("discovery", report.dump());
  report["discovered_at_ms"] =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
  atomic_write(root / "rules.json", report.dump(2) + '\n');
  // Script loading works from file:// without fetching local JSON or running a
  // server.
  atomic_write(root / "rules.js",
               "window.zucchiniRules(" + report.dump(-1, ' ', true) + ");\n");
  // HTML preview webviews commonly permit local script files while blocking
  // inline JavaScript. Keep the viewer portable without weakening their CSP.
  std::string html(dashboard);
  const auto scriptBegin = html.find("<script>");
  const auto scriptEnd = html.find("</script>", scriptBegin);
  if (scriptBegin == std::string::npos || scriptEnd == std::string::npos) {
    throw std::logic_error(
        "Rule dashboard template is missing its viewer script");
  }
  const auto codeBegin = scriptBegin + std::string("<script>").size();
  atomic_write(root / "dashboard.js",
               html.substr(codeBegin, scriptEnd - codeBegin));
  html.replace(scriptBegin,
               scriptEnd + std::string("</script>").size() - scriptBegin,
               "<script src=\"dashboard.js\"></script>");
  atomic_write(root / "index.html", html);
}

std::string rule_report_discovery_id(const std::string &directory) {
  std::ifstream stream(std::filesystem::path(directory) / "rules.json");
  if (!stream) {
    return {};
  }
  const auto report = nlohmann::json::parse(stream);
  return report.at("discovery_id").get<std::string>();
}

void store_rule_test_result(const std::string &directory,
                            const nlohmann::json &unlinkedResult) {
  auto result = unlinkedResult;
  std::string featureDirectory;
  std::ifstream discovery(std::filesystem::path(directory) / "rules.json");
  if (discovery)
    featureDirectory = nlohmann::json::parse(discovery).value(
        "feature_directory", std::string());
  for (auto &message : result.at("messages")) {
    source_location(message, featureDirectory, "file");
    if (message.contains("gherkin_location"))
      source_location(message["gherkin_location"], featureDirectory);
  }
  const auto fixture = result.at("fixture").get<std::string>();
  const auto name = result.at("test_name").get<std::string>();
  const auto key = rule_result_key(fixture, name);
  const auto base = std::filesystem::path(directory) / "results" / key;
  atomic_write(base.string() + ".json", result.dump(2) + '\n');
  atomic_write(base.string() + ".js",
               "window.zucchiniResult(" + result.dump(-1, ' ', true) + ");\n");
  const auto status = result.at("status").get<std::string>();
  if (status == "passed" || status == "failed" || status == "skipped") {
    atomic_write(base.string() + ".xml", junit(result));
  } else if (status == "started") {
    std::error_code ignored;
    std::filesystem::remove(base.string() + ".xml", ignored);
  }
}
} // namespace nZucchini
