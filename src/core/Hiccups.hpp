#pragma once

#include <string>

#include "core/StatusEffect.hpp"

namespace rogue {

// "Cegukan": dengan peluang 1/3, arah gerak diganti salah satu dari tiga
// arah lain. Dir::None tidak diubah.
class Hiccups : public StatusEffect {
 public:
  explicit Hiccups(int turns);

  std::string name() const override;
  Dir modifyMove(Dir requested, Rng& rng) override;
};

}  // namespace rogue
