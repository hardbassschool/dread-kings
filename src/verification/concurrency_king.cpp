#include "dread_kings/kings/concurrency_king.hpp"

#include <chrono>
#include <regex>
#include <sstream>

namespace dread::concurrency {

std::expected<core::VerificationVerdict, std::string>
DreadConcurrencyKing::analyze(const core::ProblemContext& context) {
  const auto start = std::chrono::steady_clock::now();
  core::VerificationVerdict verdict{.target_standard = context.requested_standard};

  const std::regex thread_pattern(R"(\bstd::(thread|jthread)\b)");
  const std::regex relaxed_store_pattern(
      R"(\.store\s*\([^,]+,\s*std::memory_order_relaxed\s*\))");

  std::istringstream stream(context.raw_source_code);
  std::string line;
  std::vector<std::string> lines;
  bool has_threads = std::regex_search(context.raw_source_code, thread_pattern);
  while (std::getline(stream, line)) {
    lines.push_back(line);
  }

  for (std::size_t index = 0; index < lines.size(); ++index) {
    const auto& source_line = lines[index];
    if (!has_threads || !std::regex_search(source_line, relaxed_store_pattern)) {
      continue;
    }

    verdict.passed = false;
    const auto store_position = source_line.find(".store");
    verdict.violations.push_back({
        .rule_id = "ISO-CONCURRENCY-RELAXED-RACE",
        .severity = core::ViolationSeverity::UndefinedBehaviorRisk,
        .message = "Atomic store uses memory_order_relaxed in a threaded context; it does not establish a release synchronizes-with edge.",
        .file_path = context.context_id,
        .line = static_cast<std::uint32_t>(index + 1),
        .column = store_position == std::string::npos
                      ? 0
                      : static_cast<std::uint32_t>(store_position + 1),
        .problematic_snippet = source_line,
        .remediation_suggestion = "Use std::memory_order_release (or a stronger ordering) when publishing payload data.",
        .iso_clause_reference = "[intro.races] / [atomics.order]"});
  }

  verdict.final_assessment = verdict.passed
      ? "No relaxed atomic publication hazards detected."
      : "A relaxed atomic publication hazard was detected.";
  verdict.evaluation_time = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now() - start);
  return verdict;
}

}  // namespace dread::concurrency