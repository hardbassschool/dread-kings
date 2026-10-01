#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace dread::discovery {
struct DesignVector {
  double mass_kg{20.0};
  double wheel_radius_m{0.20};
  double track_width_m{0.60};
  double motor_torque_nm{15.0};
  double motor_power_w{500.0};
  double battery_wh{500.0};
  double sensor_range_m{10.0};
  double sensor_rate_hz{20.0};
  double compute_w{10.0};
};

struct Objective {
  double payload_kg{20.0};
  double min_speed_mps{1.0};
  double max_energy_wh_per_m{5.0};
  double max_cost_index{100.0};
};

struct Evaluation {
  DesignVector design{};
  double estimated_payload_kg{};
  double estimated_speed_mps{};
  double energy_wh_per_m{};
  double cost_index{};
  double score{};
  bool feasible{};
};

struct DesignSpace {
  DesignVector min{};
  DesignVector max{};
};

DesignVector random_design(const DesignSpace&, std::uint64_t& state);
Evaluation evaluate(const DesignVector&, const Objective&);
std::string to_json(const Evaluation&);
}
