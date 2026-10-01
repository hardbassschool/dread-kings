#pragma once
#include "dread_kings/discovery/design_vector.hpp"
#include <string>

namespace dread::verification {
struct Report { bool passed{}; std::string message; };
Report verify(const discovery::Evaluation&, const discovery::Objective&);
}
