#include "core/Types.hpp"

#include <cstdlib>

namespace rogue {

Pos step(Pos p, Dir d) {
  switch (d) {
    case Dir::Up:
      return Pos{p.x, p.y - 1};
    case Dir::Down:
      return Pos{p.x, p.y + 1};
    case Dir::Left:
      return Pos{p.x - 1, p.y};
    case Dir::Right:
      return Pos{p.x + 1, p.y};
    case Dir::None:
      break;
  }
  return p;
}

int manhattan(Pos a, Pos b) { return std::abs(a.x - b.x) + std::abs(a.y - b.y); }

}  // namespace rogue
