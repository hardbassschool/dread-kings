#include "dread_kings/compiler/clang_rewriter.hpp"

#include <cassert>
#include <iostream>

int main() {
  const std::string source =
      "// preserve this comment\n"
      "auto* value = reinterpret_cast<int*>(ptr);\n";
  const std::vector<dread::compiler::TokenReplacementDirective> directives{
      {.line = 2,
       .column = 14,
       .target_token = "reinterpret_cast<int*>(ptr)",
       .replacement_token = "static_cast<int*>(ptr)"}};

  dread::compiler::DreadClangRewriter rewriter;
  const auto result = rewriter.apply_ast_mutation(
      "candidate.cpp", source, directives);
  assert(result.has_value());
  assert(result->replacements_count == 1);
  assert(result->modified_source ==
         "// preserve this comment\n"
         "auto* value = static_cast<int*>(ptr);\n");
  assert(result->unified_diff.find("--- a/candidate.cpp\n") == 0);
  assert(result->unified_diff.find("-auto* value = reinterpret_cast") !=
         std::string::npos);
  assert(result->unified_diff.find("+auto* value = static_cast") !=
         std::string::npos);
  std::cout << "Clang rewriter pipeline test passed.\n";
}