#pragma once

#include <string>

#include "core/Enemy.hpp"

namespace rogue {

// Ghost: hp 4, damage 3, mengejar dengan menembus dinding.
class Ghost : public Enemy {
 public:
  explicit Ghost(Pos pos);

  void act(Dungeon& dungeon, Rng& rng) override;
  std::string taunt() const override;
};

}  // namespace rogue
