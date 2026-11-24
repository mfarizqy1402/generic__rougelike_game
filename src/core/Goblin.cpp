#include "core/Goblin.hpp"

namespace rogue {

Goblin::Goblin(Pos pos) : Enemy("Goblin", 6, pos, 2) {}

void Goblin::act(Dungeon& dungeon, Rng&) { chase(dungeon, false); }

std::string Goblin::taunt() const {
  return "Goblin: Hehehe, dompetmu kelihatan berat!";
}

}  // namespace rogue
