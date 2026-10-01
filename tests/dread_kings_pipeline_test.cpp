#include "dread_kings/core/dread_core.hpp"
#include "dread_kings/kings/concurrency_king.hpp"

#include <cassert>
#include <iostream>
#include <memory>

namespace {

class PassingKing final : public dread::core::DreadKing {
public:
  [[nodiscard]] std::string_view name() const noexcept override {
    return "PassingKing";
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
  auto dread_kings = dread::core::create_dread_kings();
  dread_kings->register_king(std::make_unique<PassingKing>());
  dread_kings->register_king(
      std::make_unique<dread::concurrency::DreadConcurrencyKing>());

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

  const auto verdict = dread_kings->evaluate_all(problem);
  assert(!verdict.passed);
  assert(verdict.violations.size() == 1);
  assert(verdict.violations.front().rule_id ==
         "ISO-CONCURRENCY-RELAXED-RACE");
  assert(verdict.has_critical_failures());
  std::cout << "Dread Kings pipeline test passed.\n";
}
