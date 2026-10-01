#include "dread_kings/core/dread_core.hpp"

#include <chrono>
#include <future>
#include <sstream>

namespace dread::core {
namespace {

struct AsyncResult {
  std::string king_name;
  std::expected<VerificationVerdict, std::string> verdict;
};

class AsyncDreadKings final : public DreadKings {
public:
  void register_king(std::unique_ptr<DreadKing> king) override {
    if (king) {
      kings_.push_back(std::move(king));
    }
  }

  [[nodiscard]] VerificationVerdict evaluate_all(
      const ProblemContext& context) override {
    const auto start = std::chrono::steady_clock::now();
    std::vector<std::future<AsyncResult>> futures;
    futures.reserve(kings_.size());

    for (const auto& king : kings_) {
      DreadKing* king_ptr = king.get();
      futures.push_back(std::async(std::launch::async,
                                   [king_ptr, &context]() {
                                     return AsyncResult{
                                         std::string(king_ptr->name()),
                                         king_ptr->analyze(context)};
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
            .rule_id = "DREAD-KINGS-INTERNAL-ERROR",
            .severity = ViolationSeverity::CompilationFailure,
            .message = "Dread King '" + result.king_name + "' failed: " +
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
      assessment << "VERIFIED: All " << kings_.size() << " Dread Kings passed.";
    } else {
      assessment << "REJECTED: " << (kings_.size() - pass_count) << "/"
                 << kings_.size() << " Dread Kings flagged defects. Total violations: "
                 << combined.violations.size() << ".";
    }
    combined.final_assessment = assessment.str();
    return combined;
  }

private:
  std::vector<std::unique_ptr<DreadKing>> kings_;
};

}  // namespace

std::unique_ptr<DreadKings> create_dread_kings() {
  return std::make_unique<AsyncDreadKings>();
}

}  // namespace dread::core