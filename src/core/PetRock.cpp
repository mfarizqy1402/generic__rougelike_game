#include "core/PetRock.hpp"

#include "core/Dungeon.hpp"
#include "core/Movement.hpp"

namespace rogue {

PetRock::PetRock(Pos pos) : Entity("Pet Rock", 1, pos) {}

void PetRock::act(Dungeon& dungeon, Rng&) {
  const Entity* player = dungeon.player();
  if (player == nullptr) return;
  if (manhattan(pos(), player->pos()) > 1) {
    moveToward(dungeon, *this, player->pos(), false);
  }
}

// Batu tidak merasakan apa-apa.
void PetRock::takeDamage(int) {}

std::string PetRock::taunt() const { return "Pet Rock: ..."; }

}  // namespace rogue
