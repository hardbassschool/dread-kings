#include "dreadlords/core/dread_core.hpp"

#include <chrono>
#include <future>
#include <sstream>

namespace dread::core {
namespace {

struct AsyncResult {
  std::string lord_name;
  std::expected<VerificationVerdict, std::string> verdict;
};

class AsyncDreadCouncil final : public DreadCouncil {
public:
  void register_lord(std::unique_ptr<DreadLord> lord) override {
    if (lord) {
      lords_.push_back(std::move(lord));
    }
  }

  [[nodiscard]] VerificationVerdict evaluate_all(
      const ProblemContext& context) override {
    const auto start = std::chrono::steady_clock::now();
    std::vector<std::future<AsyncResult>> futures;
    futures.reserve(lords_.size());

    for (const auto& lord : lords_) {
      DreadLord* lord_ptr = lord.get();
      futures.push_back(std::async(std::launch::async,
                                   [lord_ptr, &context]() {
                                     return AsyncResult{
                                         std::string(lord_ptr->name()),
                                         lord_ptr->analyze(context)};
                                   }));
    }

    VerificationVerdict combined{
        .target_standard = context.requested_standard};
    std::size_t pass_count = 0;

    for (auto& future : futures) {
      auto result = future.get();
      if (!result.verdict) {
        combined.passed = false;
        combined.violations.push_back({
            .rule_id = "COUNCIL-INTERNAL-ERROR",
            .severity = ViolationSeverity::CompilationFailure,
            .message = "Lord '" + result.lord_name + "' failed: " +
                       result.verdict.error(),
            .file_path = context.context_id});
      } else if (!result.verdict->passed) {
        combined.passed = false;
        combined.violations.insert(combined.violations.end(),
                                   result.verdict->violations.begin(),
                                   result.verdict->violations.end());
      } else {
        ++pass_count;
      }
    }

    combined.evaluation_time = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start);
    std::ostringstream assessment;
    if (combined.passed) {
      assessment << "VERIFIED: All " << lords_.size() << " Lords passed.";
    } else {
      assessment << "REJECTED: " << (lords_.size() - pass_count) << "/"
                 << lords_.size() << " Lords flagged defects. Total violations: "
                 << combined.violations.size() << ".";
    }
    combined.final_assessment = assessment.str();
    return combined;
  }

private:
  std::vector<std::unique_ptr<DreadLord>> lords_;
};

}  // namespace

std::unique_ptr<DreadCouncil> create_dread_council() {
  return std::make_unique<AsyncDreadCouncil>();
}

}  // namespace dread::core