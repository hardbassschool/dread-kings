#include "dread_kings/discovery/search.hpp"
#include <algorithm>

namespace dread::discovery {
SearchResult random_search(const DesignSpace& space, const Objective& objective, const SearchConfig& cfg) {
  SearchResult r;
  r.all.reserve(cfg.samples);
  std::uint64_t state = cfg.seed;
  for (std::size_t i=0;i<cfg.samples;++i) r.all.push_back(evaluate(random_design(space,state),objective));
  std::sort(r.all.begin(), r.all.end(), [](const auto& a,const auto& b){return a.score>b.score;});
  for (const auto& e:r.all) if(e.feasible && r.feasible.size()<cfg.keep) r.feasible.push_back(e);
  return r;
}
}
