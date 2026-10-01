#include "dread_kings/robotics/robot_model.hpp"
#include <sstream>

namespace dread::robotics {
std::string RobotModel::to_sdf() const {
  std::ostringstream s;
  s << "<?xml version=\"1.0\"?>\n<sdf version=\"1.9\">\n  <model name=\"" << name << "\">\n"
    << "    <static>false</static>\n"
    << "    <!-- Dread-generated baseline differential-drive model -->\n"
    << "    <link name=\"base\"><inertial><mass>" << design.mass_kg << "</mass></inertial></link>\n"
    << "  </model>\n</sdf>\n";
  return s.str();
}
}
