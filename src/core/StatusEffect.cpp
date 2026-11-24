#include "core/StatusEffect.hpp"

namespace rogue {

StatusEffect::StatusEffect(int turns) : turnsLeft_(turns) {}

void StatusEffect::onTurnStart(Entity&) {}

bool StatusEffect::expired() const { return turnsLeft_ <= 0; }

void StatusEffect::tick() {
  if (turnsLeft_ > 0) --turnsLeft_;
}

}  // namespace rogue
