#pragma once

#include <string>

#include "core/Enemy.hpp"

namespace rogue {

// Slime: hp 10, damage 1, malas: hanya mengejar pada pemanggilan act
// ke-2, 4, 6, ...
class Slime : public Enemy {
 public:
  explicit Slime(Pos pos);

  void act(Dungeon& dungeon, Rng& rng) override;
  std::string taunt() const override;

 private:
  int actCount_ = 0;
};

}  // namespace rogue
