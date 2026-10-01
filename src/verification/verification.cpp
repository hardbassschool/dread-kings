#include "dread_kings/verification/verification.hpp"
#include <sstream>
namespace dread::verification {
Report verify(const discovery::Evaluation& e, const discovery::Objective&) {
  if (!e.feasible) return {false,"Design violates one or more declared constraints"};
  if (!(e.design.mass_kg > 0 && e.design.wheel_radius_m > 0 && e.design.motor_power_w > 0))
    return {false,"Invalid physical parameter"};
  return {true,"PASS: computational feasibility contract satisfied"};
}
}
