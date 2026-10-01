#include "dreadlords/compiler/compiler_pipeline.hpp"

#include <array>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <random>

namespace dread::compiler {
namespace {

struct Pipe {
  FILE* handle{};
  ~Pipe() {
    if (handle) {
#ifdef _WIN32
      _pclose(handle);
#else
      pclose(handle);
#endif
    }
  }
};

}  // namespace

CompilerPipeline::CompilerPipeline(std::string compiler)
    : compiler_(std::move(compiler)) {}

std::expected<CompileResult, std::string> CompilerPipeline::compile_source(
    std::string_view source_code, core::CppStandard standard,
    bool enable_sanitizers) const {
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto scratch = std::filesystem::temp_directory_path() /
                       ("dread_compile_" + std::to_string(stamp));
  const auto source_path = scratch.string() + ".cpp";
  const auto object_path = scratch.string() + ".o";

  std::ofstream source(source_path, std::ios::binary);
  if (!source) {
    return std::unexpected("Unable to create compiler scratch source.");
  }
  source << source_code;
  source.close();

  const bool needs_quotes = compiler_.find_first_of(" \t") != std::string::npos;
  const std::string compiler_command = needs_quotes ? "\"" + compiler_ + "\"" : compiler_;
  std::string command = compiler_command + " " +
                        std::string(core::to_compiler_flag(standard)) +
                        " -fsyntax-only -I\"include\"";
  if (enable_sanitizers) {
    command += " -fsanitize=address,undefined";
  }
  command += " \"" + source_path + "\" 2>&1";

#ifdef _WIN32
  Pipe pipe{_popen(command.c_str(), "r")};
#else
  Pipe pipe{popen(command.c_str(), "r")};
#endif
  if (!pipe.handle) {
    std::error_code ignored;
    std::filesystem::remove(source_path, ignored);
    return std::unexpected("Unable to launch compiler: " + compiler_);
  }

  std::array<char, 512> buffer{};
  std::string diagnostics;
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.handle)) {
    diagnostics += buffer.data();
  }
#ifdef _WIN32
  const int exit_code = _pclose(pipe.handle);
#else
  const int exit_code = pclose(pipe.handle);
#endif
  pipe.handle = nullptr;

  std::error_code ignored;
  std::filesystem::remove(source_path, ignored);
  std::filesystem::remove(object_path, ignored);
  return CompileResult{exit_code == 0, exit_code, std::move(diagnostics)};
}

}  // namespace dread::compiler