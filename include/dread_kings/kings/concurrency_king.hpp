#pragma once

#include "dread_kings/core/dread_core.hpp"

namespace dread::concurrency {

class DreadConcurrencyKing final : public core::DreadKing {
public:
  [[nodiscard]] std::string_view name() const noexcept override {
    return "DreadConcurrencyKing";
  }

  [[nodiscard]] std::string_view specialization() const noexcept override {
    return "Happens-before race detection";
  }

  [[nodiscard]] std::expected<core::VerificationVerdict, std::string> analyze(
      const core::ProblemContext& context) override;
};

}  // namespace dread::concurrency