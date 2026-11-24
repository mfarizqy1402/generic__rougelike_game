#pragma once

#include <string>

#include "core/Rng.hpp"

namespace rogue {

// Peramal tidak berguna: memilih satu ramalan lucu secara acak.
class FortuneTeller {
 public:
  std::string fortune(Rng& rng) const;
};

}  // namespace rogue
