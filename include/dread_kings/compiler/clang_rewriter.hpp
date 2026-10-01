#pragma once

#include "dread_kings/core/dread_core.hpp"

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace dread::compiler {

struct SurgicalPatch {
  std::string file_path;
  std::string original_source;
  std::string modified_source;
  std::string unified_diff;
  std::uint32_t replacements_count{};
};

struct TokenReplacementDirective {
  std::uint32_t line{};
  std::uint32_t column{};
  std::string target_token;
  std::string replacement_token;
};

class DreadClangRewriter {
public:
  [[nodiscard]] std::expected<SurgicalPatch, std::string> apply_ast_mutation(
      std::string_view file_name, std::string_view source_code,
      const std::vector<TokenReplacementDirective>& directives,
      core::CppStandard standard = core::CppStandard::Cpp23) const;

  [[nodiscard]] static std::string generate_unified_diff(
      std::string_view file_path, std::string_view original,
      std::string_view modified);
};

}  // namespace dread::compiler