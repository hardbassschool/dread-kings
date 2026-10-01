#pragma once

#include "dreadlords/core/dread_core.hpp"

#include <expected>
#include <string>
#include <string_view>

namespace dread::compiler {

struct CompileResult {
  bool success{false};
  int exit_code{-1};
  std::string stderr_log;
};

class CompilerPipeline {
public:
  explicit CompilerPipeline(std::string compiler = "c++");

  [[nodiscard]] std::expected<CompileResult, std::string> compile_source(
      std::string_view source_code, core::CppStandard standard,
      bool enable_sanitizers = false) const;

private:
  std::string compiler_;
};

}  // namespace dread::compiler