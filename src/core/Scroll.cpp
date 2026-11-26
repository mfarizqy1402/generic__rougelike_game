#include "core/Scroll.hpp"

#include <memory>

#include "core/Hiccups.hpp"
#include "core/Player.hpp"

namespace rogue {

Scroll::Scroll() : Item("Gulungan Oleng") {}

std::string Scroll::use(Player& player) {
  player.addStatus(std::make_unique<Hiccups>(kHiccupTurns));
  return "Kamu membaca Gulungan Oleng. Hik! Tiba-tiba kamu cegukan.";
}

}  // namespace rogue
