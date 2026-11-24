#pragma once

#include <string>

#include "core/Enemy.hpp"

namespace rogue {

// Goblin: hp 6, damage 2, mengejar setiap giliran.
class Goblin : public Enemy {
 public:
  explicit Goblin(Pos pos);

  void act(Dungeon& dungeon, Rng& rng) override;
  std::string taunt() const override;
};

}  // namespace rogue
