#include "dreadlords/discovery/search.hpp"
#include "dreadlords/compiler/compiler_pipeline.hpp"
#include "dreadlords/core/dread_core.hpp"
#include "dreadlords/lords/concurrency_lord.hpp"
#include "dreadlords/robotics/robot_model.hpp"
#include "dreadlords/toolchain/toolchain.hpp"
#include "dreadlords/verification/verification.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>

namespace {

int verify_file(const char* file_path, dread::core::CppStandard standard) {
  std::ifstream input(file_path, std::ios::binary);
  if (!input) {
    std::cerr << "Unable to open source file: " << file_path << '\n';
    return 1;
  }

  std::ostringstream source;
  source << input.rdbuf();
  dread::core::ProblemContext context{
      .context_id = file_path,
      .raw_source_code = source.str(),
      .requested_standard = standard};
  auto council = dread::core::create_dread_council();
  council->register_lord(
      std::make_unique<dread::concurrency::DreadConcurrencyLord>());
  const auto verdict = council->evaluate_all(context);

  std::cout << "Verification: " << (verdict.passed ? "PASS" : "REJECTED")
            << "\n" << verdict.final_assessment << '\n';
  for (const auto& violation : verdict.violations) {
    std::cout << violation.rule_id << " at " << violation.line << ':'
              << violation.column << ": " << violation.message << '\n';
    std::cout << "Remediation: " << violation.remediation_suggestion << '\n';
  }

  const char* configured_compiler = std::getenv("DREAD_COMPILER");
    const std::string compiler_name =
      configured_compiler ? configured_compiler : "clang++";
  dread::compiler::CompilerPipeline compiler(
      compiler_name);
  const auto compile_result = compiler.compile_source(
      context.raw_source_code, context.requested_standard);
  if (!compile_result) {
    std::cout << "Compiler check unavailable: " << compile_result.error() << '\n';
  } else if (!compile_result->success) {
    const bool compiler_missing =
        compile_result->exit_code == 9009 &&
        compile_result->stderr_log.find("not recognized") != std::string::npos;
    if (compiler_missing) {
      std::cout << "Compiler check unavailable: " << compiler_name
                << " is not available.\n";
    } else {
      std::cout << "Compiler: FAIL\n" << compile_result->stderr_log;
    }
    if (compiler_missing) {
      return verdict.passed ? 0 : 1;
    }
  } else {
    std::cout << "Compiler: PASS (clang++)\n";
  }
  return verdict.passed && (!compile_result || compile_result->success) ? 0 : 1;
}

}  // namespace

int main(int argc, char* argv[]) {
  using namespace dread;
  if (argc >= 3 && std::string_view(argv[1]) == "verify") {
    const bool has_standard = argc >= 4 &&
                              std::string_view(argv[2]).starts_with("--std=");
    const char* file_path = has_standard ? argv[3] : argv[2];
    const auto standard = has_standard && std::string_view(argv[2]) == "--std=c++20"
                              ? core::CppStandard::Cpp20
                              : core::CppStandard::Cpp23;
    return verify_file(file_path, standard);
  }
  discovery::DesignSpace space{
    {10,0.10,0.30,5,200,100,3,5,2},
    {100,0.50,1.20,60,3000,2000,30,100,100}
  };
  discovery::Objective objective{20,1.0,5.0,100};
  discovery::SearchConfig cfg{50000,10,1234567};
  const auto result=discovery::random_search(space,objective,cfg);
  std::cout << "DREAD LORDS v3\n";
  std::cout << "Compiler: " << toolchain::probe().version << "\n";
  std::cout << "Candidates evaluated: " << result.all.size() << "\n";
  std::cout << "Feasible candidates retained: " << result.feasible.size() << "\n\n";
  for(std::size_t i=0;i<result.feasible.size();++i){
    const auto& e=result.feasible[i];
    auto report=verification::verify(e,objective);
    std::cout << "#"<<i+1<<" score="<<e.score<<" payload="<<e.estimated_payload_kg
              <<"kg speed="<<e.estimated_speed_mps<<"m/s energy="<<e.energy_wh_per_m
              <<" Wh/m verification="<<(report.passed?"PASS":"FAIL")<<"\n";
  }
  if(!result.feasible.empty()) {
    robotics::RobotModel model{"DREAD-Candidate-1",result.feasible.front().design};
    std::cout << "\nGenerated SDF:\n" << model.to_sdf();
  }
}
