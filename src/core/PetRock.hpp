#pragma once

#include <string>

#include "core/Entity.hpp"

namespace rogue {

// Pet Rock: batu peliharaan yang mengikuti pemain sampai menempel.
// Tidak bisa dilukai dan tidak melakukan hal lain.
class PetRock : public Entity {
 public:
  explicit PetRock(Pos pos);

  void act(Dungeon& dungeon, Rng& rng) override;
  void takeDamage(int amount) override;
  std::string taunt() const override;
};

}  // namespace rogue
