#include "core/Chest.hpp"

#include <utility>

namespace rogue {

Chest::Chest(Pos pos, std::vector<std::shared_ptr<Item>> loot)
    : Entity("Peti", kHp, pos), loot_(std::move(loot)) {}

void Chest::act(Dungeon&, Rng&) {}

const std::vector<std::shared_ptr<Item>>& Chest::loot() const { return loot_; }

std::vector<std::shared_ptr<Item>> Chest::takeLoot() {
  std::vector<std::shared_ptr<Item>> taken = std::move(loot_);
  loot_.clear();
  return taken;
}

}  // namespace rogue
