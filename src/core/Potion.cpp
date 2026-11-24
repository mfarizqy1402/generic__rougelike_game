#include "core/Potion.hpp"

#include "core/Player.hpp"

namespace rogue {

Potion::Potion() : Item("Ramuan") {}

std::string Potion::use(Player& player) {
  const int before = player.hp();
  player.heal(kHealAmount);
  const int gained = player.hp() - before;
  return "Kamu meminum Ramuan. Rasanya aneh, tapi manjur. HP +" +
         std::to_string(gained) + ".";
}

}  // namespace rogue
