#include "core/Hiccups.hpp"

#include <random>

namespace rogue {

Hiccups::Hiccups(int turns) : StatusEffect(turns) {}

std::string Hiccups::name() const { return "Cegukan"; }

Dir Hiccups::modifyMove(Dir requested, Rng& rng) {
  if (requested == Dir::None) return requested;

  // Lempar "dadu" 0..2; hanya hasil 0 (peluang 1/3) yang mengganti arah.
  if (std::uniform_int_distribution<int>(0, 2)(rng) != 0) return requested;

  const Dir all[] = {Dir::Up, Dir::Down, Dir::Left, Dir::Right};
  Dir others[3];
  int count = 0;
  for (Dir candidate : all) {
    if (candidate != requested) others[count++] = candidate;
  }
  return others[std::uniform_int_distribution<int>(0, 2)(rng)];
}

}  // namespace rogue
