#include "core/Slime.hpp"

namespace rogue {

Slime::Slime(Pos pos) : Enemy("Slime", 10, pos, 1) {}

void Slime::act(Dungeon& dungeon, Rng&) {
  ++actCount_;
  if (actCount_ % 2 == 0) {
    chase(dungeon, false);
  } else {
    clearAttack();  // giliran istirahat
  }
}

std::string Slime::taunt() const { return "Slime: Plop. Plop. Blub."; }

}  // namespace rogue
