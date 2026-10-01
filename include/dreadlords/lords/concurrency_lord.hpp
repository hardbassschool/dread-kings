#pragma once

#include "dreadlords/core/dread_core.hpp"

namespace dread::concurrency {

class DreadConcurrencyLord final : public core::DreadLord {
public:
  [[nodiscard]] std::string_view name() const noexcept override {
    return "DreadConcurrencyLord";
  }

  [[nodiscard]] std::string_view specialization() const noexcept override {
    return "Happens-before race detection";
  }

  [[nodiscard]] std::expected<core::VerificationVerdict, std::string> analyze(
      const core::ProblemContext& context) override;
};

}  // namespace dread::concurrency