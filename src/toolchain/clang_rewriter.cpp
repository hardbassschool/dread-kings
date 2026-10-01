#include "dreadlords/compiler/clang_rewriter.hpp"

#include <algorithm>
#include <sstream>

namespace dread::compiler {
namespace {

std::vector<std::string> split_lines(std::string_view source) {
  std::vector<std::string> lines;
  std::istringstream stream{std::string(source)};
  std::string line;
  while (std::getline(stream, line)) {
    lines.push_back(std::move(line));
  }
  if (!source.empty() && source.back() == '\n') {
    lines.emplace_back();
  }
  return lines;
}

}  // namespace

std::expected<SurgicalPatch, std::string> DreadClangRewriter::apply_ast_mutation(
    std::string_view file_name, std::string_view source_code,
    const std::vector<TokenReplacementDirective>& directives,
    core::CppStandard) const {
  std::string modified(source_code);
  std::uint32_t replacement_count = 0;

  for (const auto& directive : directives) {
    if (directive.line == 0 || directive.target_token.empty()) {
      return std::unexpected("Each replacement needs a line and target token.");
    }
    std::size_t line_start = 0;
    for (std::uint32_t line = 1; line < directive.line; ++line) {
      const auto newline = modified.find('\n', line_start);
      if (newline == std::string::npos) {
        return std::unexpected("Replacement line is outside the source file.");
      }
      line_start = newline + 1;
    }
    const auto line_end = modified.find('\n', line_start);
    const auto line_length = line_end == std::string::npos
                                 ? modified.size() - line_start
                                 : line_end - line_start;
    if (directive.column == 0 || directive.column > line_length + 1) {
      return std::unexpected("Replacement column is outside the source line.");
    }
    const auto token_start = line_start + directive.column - 1;
    const auto token_position = modified.find(directive.target_token, token_start);
    if (token_position == std::string::npos ||
        (line_end != std::string::npos && token_position >= line_end)) {
      return std::unexpected("Target token was not found at the requested line.");
    }
    modified.replace(token_position, directive.target_token.size(),
                     directive.replacement_token);
    ++replacement_count;
  }

  const auto diff = generate_unified_diff(file_name, source_code, modified);
  return SurgicalPatch{.file_path = std::string(file_name),
                       .original_source = std::string(source_code),
                       .modified_source = std::move(modified),
                       .unified_diff = diff,
                       .replacements_count = replacement_count};
}

std::string DreadClangRewriter::generate_unified_diff(
    std::string_view file_path, std::string_view original,
    std::string_view modified) {
  const auto before = split_lines(original);
  const auto after = split_lines(modified);
  std::ostringstream diff;
  diff << "--- a/" << file_path << '\n' << "+++ b/" << file_path << '\n';

  const auto count = std::max(before.size(), after.size());
  for (std::size_t index = 0; index < count; ++index) {
    const std::string old_line = index < before.size() ? before[index] : "";
    const std::string new_line = index < after.size() ? after[index] : "";
    if (old_line != new_line) {
      diff << "@@ -" << index + 1 << ",1 +" << index + 1 << ",1 @@\n";
      diff << '-' << old_line << '\n' << '+' << new_line << '\n';
    }
  }
  return diff.str();
}

}  // namespace dread::compiler