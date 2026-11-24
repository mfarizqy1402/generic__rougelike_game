#pragma once

#include "core/Types.hpp"

namespace rogue {

class Dungeon;
class Entity;

// Satu langkah self menuju target. Sumbu dengan selisih terbesar dicoba lebih
// dulu (seri: sumbu x), lalu sumbu lainnya. Tujuan langkah harus di dalam peta,
// tidak ditempati entity hidup, dan (kecuali throughWalls) walkable.
// Bila avoidSafe, petak zona aman (Tile::isSafe) juga ditolak; sumbu lain
// dicoba lebih dulu. Mengembalikan true bila self berpindah.
bool moveToward(Dungeon& dungeon, Entity& self, Pos target, bool throughWalls,
                bool avoidSafe = false);

}  // namespace rogue
