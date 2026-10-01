#pragma once

#include <chrono>
#include <cstdint>
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace dread::core {

enum class CppStandard : std::uint8_t {
  Cpp11,
  Cpp14,
  Cpp17,
  Cpp20,
  Cpp23,
  Cpp26
};

[[nodiscard]] constexpr std::string_view to_string(CppStandard standard) noexcept {
  switch (standard) {
    case CppStandard::Cpp11: return "C++11";
    case CppStandard::Cpp14: return "C++14";
    case CppStandard::Cpp17: return "C++17";
    case CppStandard::Cpp20: return "C++20";
    case CppStandard::Cpp23: return "C++23";
    case CppStandard::Cpp26: return "C++26";
  }
  return "C++23";
}

[[nodiscard]] constexpr std::string_view to_compiler_flag(
    CppStandard standard) noexcept {
  switch (standard) {
    case CppStandard::Cpp11: return "-std=c++11";
    case CppStandard::Cpp14: return "-std=c++14";
    case CppStandard::Cpp17: return "-std=c++17";
    case CppStandard::Cpp20: return "-std=c++20";
    case CppStandard::Cpp23: return "-std=c++23";
    case CppStandard::Cpp26: return "-std=c++2c";
  }
  return "-std=c++23";
}

enum class ViolationSeverity {
  Warning,
  CompilationFailure,
  UndefinedBehaviorRisk
};

struct Violation {
  std::string rule_id;
  ViolationSeverity severity{ViolationSeverity::Warning};
  std::string message;
  std::string file_path;
  std::uint32_t line{};
  std::uint32_t column{};
  std::string problematic_snippet;
  std::string remediation_suggestion;
  std::string iso_clause_reference;
};

struct VerificationVerdict {
  bool passed{true};
  CppStandard target_standard{CppStandard::Cpp23};
  std::vector<Violation> violations;
  std::chrono::microseconds evaluation_time{};
  std::string final_assessment;

  [[nodiscard]] bool has_critical_failures() const noexcept {
    for (const auto& violation : violations) {
      if (violation.severity == ViolationSeverity::CompilationFailure ||
          violation.severity == ViolationSeverity::UndefinedBehaviorRisk) {
        return true;
      }
    }
    return false;
  }
};

struct ProblemContext {
  std::string context_id;
  std::string raw_source_code;
  CppStandard requested_standard{CppStandard::Cpp23};
};

class DreadLord {
public:
  virtual ~DreadLord() = default;
  virtual std::string_view name() const noexcept = 0;
  virtual std::string_view specialization() const noexcept = 0;
  virtual std::expected<VerificationVerdict, std::string> analyze(
      const ProblemContext& context) = 0;
};

class DreadCouncil {
public:
  virtual ~DreadCouncil() = default;
  virtual void register_lord(std::unique_ptr<DreadLord> lord) = 0;
  [[nodiscard]] virtual VerificationVerdict evaluate_all(
      const ProblemContext& context) = 0;
};

[[nodiscard]] std::unique_ptr<DreadCouncil> create_dread_council();

}  // namespace dread::core