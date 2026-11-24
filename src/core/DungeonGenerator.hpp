#pragma once

#include "core/Dungeon.hpp"
#include "core/Rng.hpp"

namespace rogue {

// Membuat peta berisi ruang-ruang persegi yang dihubungkan koridor berbentuk L.
class DungeonGenerator {
 public:
  // Syarat: w >= 12 dan h >= 10, selain itu std::invalid_argument.
  // Seed Rng yang sama menghasilkan peta yang sama.
  Dungeon generate(int w, int h, Rng& rng) const;
};

}  // namespace rogue
