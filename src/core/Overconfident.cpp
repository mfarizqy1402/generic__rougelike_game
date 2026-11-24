#include "core/Overconfident.hpp"

namespace rogue {

namespace {

Dir opposite(Dir d) {
  switch (d) {
    case Dir::Up:
      return Dir::Down;
    case Dir::Down:
      return Dir::Up;
    case Dir::Left:
      return Dir::Right;
    case Dir::Right:
      return Dir::Left;
    case Dir::None:
      break;
  }
  return Dir::None;
}

}  // namespace

Overconfident::Overconfident(int turns) : StatusEffect(turns) {}

std::string Overconfident::name() const { return "Terlalu Percaya Diri"; }

Dir Overconfident::modifyMove(Dir requested, Rng&) {
  if (requested == Dir::None) return Dir::None;
  if (lastMove_ != Dir::None && requested == opposite(lastMove_)) {
    return Dir::None;  // menolak mundur; gerak terakhir tidak berubah
  }
  lastMove_ = requested;
  return requested;
}

}  // namespace rogue
