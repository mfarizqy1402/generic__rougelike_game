#include "core/Enemy.hpp"

#include <utility>

#include "core/Dungeon.hpp"
#include "core/Movement.hpp"

namespace rogue {

Enemy::Enemy(std::string name, int hp, Pos pos, int damage)
    : Entity(std::move(name), hp, pos), damage_(damage) {}

int Enemy::damage() const { return damage_; }

bool Enemy::attackedLastTurn() const { return attacked_; }

void Enemy::clearAttack() { attacked_ = false; }

void Enemy::chase(Dungeon& dungeon, bool throughWalls) {
  attacked_ = false;
  if (!isAlive()) return;

  Entity* target = dungeon.player();
  if (target == nullptr || !target->isAlive()) return;

  if (manhattan(pos(), target->pos()) == 1) {
    target->takeDamage(damage_);
    attacked_ = true;
    return;
  }
  moveToward(dungeon, *this, target->pos(), throughWalls, true);
}

}  // namespace rogue
