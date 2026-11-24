#include "core/Player.hpp"

#include <algorithm>
#include <utility>

namespace rogue {

Player::Player(std::string name, Pos pos)
    : Entity(std::move(name), kMaxHp, pos) {}

void Player::act(Dungeon&, Rng&) {}

void Player::heal(int amount) {
  setHp(std::min(kMaxHp, hp() + std::max(0, amount)));
}

std::vector<std::shared_ptr<Item>>& Player::inventory() { return inventory_; }
const std::vector<std::shared_ptr<Item>>& Player::inventory() const {
  return inventory_;
}

int Player::depth() const { return depth_; }
void Player::setDepth(int depth) { depth_ = depth; }

}  // namespace rogue
