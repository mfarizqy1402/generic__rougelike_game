#pragma once

namespace rogue {

// Koordinat petak. Sumbu y bertambah ke bawah (seperti baris di layar).
struct Pos {
  int x = 0;
  int y = 0;
};

inline bool operator==(const Pos& a, const Pos& b) {
  return a.x == b.x && a.y == b.y;
}
inline bool operator!=(const Pos& a, const Pos& b) { return !(a == b); }

enum class Dir { Up, Down, Left, Right, None };

// Satu langkah dari p menuju d. Dir::None berarti tetap di tempat.
Pos step(Pos p, Dir d);

// Jarak Manhattan: |dx| + |dy|.
int manhattan(Pos a, Pos b);

}  // namespace rogue
