#include "dreadlords/discovery/search.hpp"
#include "dreadlords/verification/verification.hpp"
#include <cassert>
#include <iostream>
int main(){
  dread::discovery::DesignSpace s{{10,.1,.3,5,200,100,3,5,2},{100,.5,1.2,60,3000,2000,30,100,100}};
  dread::discovery::Objective o{20,1,5,100};
  auto r=dread::discovery::random_search(s,o,{2000,5,42});
  assert(r.all.size()==2000);
  assert(!r.feasible.empty());
  assert(dread::verification::verify(r.feasible.front(),o).passed);
  std::cout<<"All Dread Lords core tests passed.\n";
}
