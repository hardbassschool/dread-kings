#include "dreadlords/discovery/design_vector.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace dread::discovery {
namespace {
double unit(std::uint64_t& s) {
  s ^= s << 13; s ^= s >> 7; s ^= s << 17;
  return static_cast<double>(s % 1000000ULL) / 999999.0;
}
double lerp(double a, double b, double t) { return a + (b-a)*t; }
}

DesignVector random_design(const DesignSpace& x, std::uint64_t& s) {
  return {
    lerp(x.min.mass_kg,x.max.mass_kg,unit(s)),
    lerp(x.min.wheel_radius_m,x.max.wheel_radius_m,unit(s)),
    lerp(x.min.track_width_m,x.max.track_width_m,unit(s)),
    lerp(x.min.motor_torque_nm,x.max.motor_torque_nm,unit(s)),
    lerp(x.min.motor_power_w,x.max.motor_power_w,unit(s)),
    lerp(x.min.battery_wh,x.max.battery_wh,unit(s)),
    lerp(x.min.sensor_range_m,x.max.sensor_range_m,unit(s)),
    lerp(x.min.sensor_rate_hz,x.max.sensor_rate_hz,unit(s)),
    lerp(x.min.compute_w,x.max.compute_w,unit(s))
  };
}

Evaluation evaluate(const DesignVector& d, const Objective& o) {
  const double traction = std::sqrt(std::max(0.0,d.motor_torque_nm)) * 3.0;
  const double power_speed = std::cbrt(std::max(1.0,d.motor_power_w)) * 0.18;
  const double speed = std::clamp(traction * 0.05 + power_speed, 0.0, 8.0);
  const double payload = std::max(0.0, d.motor_torque_nm * d.wheel_radius_m * 2.5 - d.mass_kg * 0.05);
  const double energy = (d.motor_power_w + d.compute_w + d.sensor_rate_hz * 0.15) /
                        std::max(0.1, speed * 100.0);
  const double cost = d.motor_power_w * 0.04 + d.battery_wh * 0.02 + d.sensor_range_m * 1.5 + d.mass_kg;
  const bool feasible = payload >= o.payload_kg && speed >= o.min_speed_mps &&
                        energy <= o.max_energy_wh_per_m && cost <= o.max_cost_index;
  const double score = (payload * std::max(speed,0.01)) /
                       (std::max(energy,0.01) * std::max(cost,1.0));
  return {d,payload,speed,energy,cost,score,feasible};
}

std::string to_json(const Evaluation& e) {
  std::ostringstream out; out << std::setprecision(8);
  out << "{\n  \"feasible\": " << (e.feasible?"true":"false")
      << ",\n  \"score\": " << e.score
      << ",\n  \"payload_kg\": " << e.estimated_payload_kg
      << ",\n  \"speed_mps\": " << e.estimated_speed_mps
      << ",\n  \"energy_wh_per_m\": " << e.energy_wh_per_m
      << ",\n  \"cost_index\": " << e.cost_index << "\n}";
  return out.str();
}
}
