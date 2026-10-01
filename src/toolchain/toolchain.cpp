#include "dreadlords/toolchain/toolchain.hpp"
#include <cstdlib>
#include <array>
#include <cstdio>
#include <memory>
namespace dread::toolchain {
Report probe() {
  std::array<char,256> b{}; std::string out;
#ifdef _WIN32
  FILE* p=_popen("c++ --version 2>NUL","r");
#else
  FILE* p=popen("c++ --version 2>/dev/null","r");
#endif
  if(p){
    while(fgets(b.data(),static_cast<int>(b.size()),p)) out+=b.data();
#ifdef _WIN32
    _pclose(p);
#else
    pclose(p);
#endif
  }
  const auto nl=out.find('\n'); if(nl!=std::string::npos) out.resize(nl);
  return {"c++",out,"C++23"};
}
}
