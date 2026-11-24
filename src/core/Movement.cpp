#include "core/Movement.hpp"

#include <cstdlib>
#include <utility>

#include "core/Dungeon.hpp"
#include "core/Entity.hpp"

namespace rogue {

bool moveToward(Dungeon& dungeon, Entity& self, Pos target, bool throughWalls,
                bool avoidSafe) {
  const Pos from = self.pos();
  const int dx = target.x - from.x;
  const int dy = target.y - from.y;
  if (dx == 0 && dy == 0) return false;

  Dir horizontal = Dir::None;
  if (dx > 0) horizontal = Dir::Right;
  if (dx < 0) horizontal = Dir::Left;
  Dir vertical = Dir::None;
  if (dy > 0) vertical = Dir::Down;
  if (dy < 0) vertical = Dir::Up;

  // Sumbu dengan selisih terbesar dulu; seri berarti sumbu x.
  Dir first = horizontal;
  Dir second = vertical;
  if (std::abs(dy) > std::abs(dx)) std::swap(first, second);

  for (Dir dir : {first, second}) {
    if (dir == Dir::None) continue;
    const Pos dest = step(from, dir);
    if (!dungeon.inBounds(dest)) continue;
    if (dungeon.entityAt(dest) != nullptr) continue;
    if (!throughWalls && !dungeon.tile(dest).isWalkable()) continue;
    if (avoidSafe && dungeon.tile(dest).isSafe()) continue;
    self.setPos(dest);
    return true;
  }
  return false;
}

}  // namespace rogue
