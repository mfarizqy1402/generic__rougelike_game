#include "core/Ghost.hpp"

namespace rogue {

Ghost::Ghost(Pos pos) : Enemy("Ghost", 4, pos, 3) {}

void Ghost::act(Dungeon& dungeon, Rng&) { chase(dungeon, true); }

std::string Ghost::taunt() const {
  return "Ghost: Uuuuh... aku lupa cara menakut-nakuti.";
}

}  // namespace rogue
