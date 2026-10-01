#pragma once
#include "dreadlords/discovery/design_vector.hpp"
#include <string>

namespace dread::robotics {
struct RobotModel {
  std::string name{"DREAD-Discovery-1"};
  discovery::DesignVector design{};
  std::string to_sdf() const;
};
}
