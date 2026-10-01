#pragma once
#include <string>
namespace dread::toolchain { struct Report { std::string compiler; std::string version; std::string standard; }; Report probe(); }
