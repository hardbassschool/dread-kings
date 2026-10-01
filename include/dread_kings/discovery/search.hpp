#pragma once
#include "dread_kings/discovery/design_vector.hpp"
#include <cstddef>
#include <vector>

namespace dread::discovery {
struct SearchConfig { std::size_t samples{10000}; std::size_t keep{20}; unsigned long long seed{88172645463325252ULL}; };
struct SearchResult { std::vector<Evaluation> feasible; std::vector<Evaluation> all; };
SearchResult random_search(const DesignSpace&, const Objective&, const SearchConfig&);
}
