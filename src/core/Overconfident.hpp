#pragma once

#include <string>

#include "core/StatusEffect.hpp"

namespace rogue {

// "Terlalu Percaya Diri": menolak mundur. Arah yang berlawanan dengan arah
// gerak terakhir diubah menjadi Dir::None.
class Overconfident : public StatusEffect {
 public:
  explicit Overconfident(int turns);

  std::string name() const override;
  Dir modifyMove(Dir requested, Rng& rng) override;

 private:
  Dir lastMove_ = Dir::None;
};

}  // namespace rogue
