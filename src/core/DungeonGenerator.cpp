#include "core/DungeonGenerator.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

#include "core/Tile.hpp"
#include "core/Types.hpp"

namespace rogue {

namespace {

constexpr int kMinWidth = 12;
constexpr int kMinHeight = 10;
constexpr int kMaxAttempts = 60;
constexpr int kLootRoomChance = 60;  // persen

struct Room {
  int x;
  int y;
  int w;
  int h;

  Pos center() const { return Pos{x + w / 2, y + h / 2}; }
};

// Dua ruang dianggap tidak menempel bila ada jarak minimal 1 petak di antaranya.
bool separated(const Room& a, const Room& b) {
  return a.x + a.w + 1 <= b.x || b.x + b.w + 1 <= a.x ||
         a.y + a.h + 1 <= b.y || b.y + b.h + 1 <= a.y;
}

int randomBetween(Rng& rng, int lo, int hi) {
  return std::uniform_int_distribution<int>(lo, hi)(rng);
}

void carveRoom(Dungeon& d, const Room& room) {
  for (int y = room.y; y < room.y + room.h; ++y) {
    for (int x = room.x; x < room.x + room.w; ++x) {
      d.tile(Pos{x, y}).setType(Floor);
    }
  }
}

// Koridor L: horizontal dari a ke b.x, lalu vertikal ke b.y.
void carveCorridor(Dungeon& d, Pos a, Pos b) {
  const int dx = (b.x >= a.x) ? 1 : -1;
  for (int x = a.x;; x += dx) {
    d.tile(Pos{x, a.y}).setType(Floor);
    if (x == b.x) break;
  }
  const int dy = (b.y >= a.y) ? 1 : -1;
  for (int y = a.y;; y += dy) {
    d.tile(Pos{b.x, y}).setType(Floor);
    if (y == b.y) break;
  }
}

}  // namespace

Dungeon DungeonGenerator::generate(int w, int h, Rng& rng) const {
  if (w < kMinWidth || h < kMinHeight) {
    throw std::invalid_argument(
        "Ukuran peta minimal 12x10 petak.");
  }

  Dungeon dungeon(w, h);

  // Coba taruh ruang acak 4..9 x 3..6 di dalam area 1..w-2 / 1..h-2.
  std::vector<Room> rooms;
  for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
    const int rw = randomBetween(rng, 4, 9);
    const int rh = randomBetween(rng, 3, 6);
    const int rx = randomBetween(rng, 1, w - 1 - rw);
    const int ry = randomBetween(rng, 1, h - 1 - rh);
    const Room candidate{rx, ry, rw, rh};

    const bool ok = std::all_of(rooms.begin(), rooms.end(),
                                [&](const Room& other) {
                                  return separated(candidate, other);
                                });
    if (ok) rooms.push_back(candidate);
  }

  // Cadangan: dua ruang tetap bila percobaan acak kurang dari 2 ruang.
  if (rooms.size() < 2) {
    rooms.clear();
    rooms.push_back(Room{1, 1, 4, 3});
    rooms.push_back(Room{w - 5, h - 4, 4, 3});
  }

  for (const Room& room : rooms) carveRoom(dungeon, room);

  for (size_t i = 1; i < rooms.size(); ++i) {
    carveCorridor(dungeon, rooms[i - 1].center(), rooms[i].center());
  }

  // Ruang loot: satu ruang tengah (bukan ruang awal/tangga) menjadi zona aman.
  // Lemparan acak dilakukan setelah semua ruang dan koridor jadi, sehingga
  // tata letak untuk seed yang sama tidak berubah.
  if (rooms.size() >= 3 && randomBetween(rng, 1, 100) <= kLootRoomChance) {
    const int index =
        randomBetween(rng, 1, static_cast<int>(rooms.size()) - 2);
    const Room& loot = rooms[static_cast<size_t>(index)];
    for (int y = loot.y; y < loot.y + loot.h; ++y) {
      for (int x = loot.x; x < loot.x + loot.w; ++x) {
        dungeon.tile(Pos{x, y}).setSafe(true);
      }
    }
    dungeon.setLootCenter(loot.center());
  }

  // Tangga diset paling akhir supaya tidak tertimpa koridor.
  dungeon.setStart(rooms.front().center());
  dungeon.setStairs(rooms.back().center());
  dungeon.tile(dungeon.stairs()).setType(Stairs);

  return dungeon;
}

}  // namespace rogue
