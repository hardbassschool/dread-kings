#include "dreadlords/core/dread_core.hpp"
#include "dreadlords/lords/concurrency_lord.hpp"

#include <cassert>
#include <iostream>
#include <memory>

namespace {

class PassingLord final : public dread::core::DreadLord {
public:
  [[nodiscard]] std::string_view name() const noexcept override {
    return "PassingLord";
  }

  [[nodiscard]] std::string_view specialization() const noexcept override {
    return "test";
  }

  [[nodiscard]] std::expected<dread::core::VerificationVerdict, std::string>
  analyze(const dread::core::ProblemContext& context) override {
    return dread::core::VerificationVerdict{
        .target_standard = context.requested_standard};
  }
};

}  // namespace

int main() {
  auto council = dread::core::create_dread_council();
  council->register_lord(std::make_unique<PassingLord>());
  council->register_lord(
      std::make_unique<dread::concurrency::DreadConcurrencyLord>());

  const dread::core::ProblemContext problem{
      .context_id = "lock_free_queue.cpp",
      .raw_source_code = R"cpp(
#include <atomic>
#include <thread>
std::atomic<bool> ready{false};
int payload = 0;
void publish(int value) {
  payload = value;
  ready.store(true, std::memory_order_relaxed);
}
)cpp"};

  const auto verdict = council->evaluate_all(problem);
  assert(!verdict.passed);
  assert(verdict.violations.size() == 1);
  assert(verdict.violations.front().rule_id ==
         "ISO-CONCURRENCY-RELAXED-RACE");
  assert(verdict.has_critical_failures());
  std::cout << "Council pipeline test passed.\n";
}
