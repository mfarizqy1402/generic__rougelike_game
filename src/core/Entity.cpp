#include "core/Entity.hpp"

#include <algorithm>
#include <utility>

namespace rogue {

Entity::Entity(std::string name, int hp, Pos pos)
    : name_(std::move(name)), hp_(std::max(0, hp)), pos_(pos) {}

Entity::~Entity() = default;

std::string Entity::taunt() const { return ""; }

void Entity::takeDamage(int amount) {
  hp_ = std::max(0, hp_ - std::max(0, amount));
}

const std::string& Entity::name() const { return name_; }
int Entity::hp() const { return hp_; }
bool Entity::isAlive() const { return hp_ > 0; }
Pos Entity::pos() const { return pos_; }
void Entity::setPos(Pos pos) { pos_ = pos; }

void Entity::addStatus(std::unique_ptr<StatusEffect> status) {
  if (status) statuses_.push_back(std::move(status));
}

std::vector<std::unique_ptr<StatusEffect>>& Entity::statuses() {
  return statuses_;
}
const std::vector<std::unique_ptr<StatusEffect>>& Entity::statuses() const {
  return statuses_;
}

void Entity::setHp(int hp) { hp_ = std::max(0, hp); }

}  // namespace rogue
