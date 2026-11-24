// Tes DungeonGenerator: deterministik, terhubung, tepi tetap Wall.
#include <cstdint>
#include <queue>
#include <stdexcept>
#include <vector>

#include "check.hpp"
#include "core/Dungeon.hpp"
#include "core/DungeonGenerator.hpp"
#include "core/Rng.hpp"
#include "core/Tile.hpp"
#include "core/Types.hpp"

using namespace rogue;

// BFS 4 arah lewat petak walkable.
static bool reachable(const Dungeon& d, Pos from, Pos to) {
  std::vector<char> seen(static_cast<size_t>(d.width() * d.height()), 0);
  std::queue<Pos> frontier;
  frontier.push(from);
  seen[static_cast<size_t>(from.y * d.width() + from.x)] = 1;
  const Dir dirs[] = {Dir::Up, Dir::Down, Dir::Left, Dir::Right};
  while (!frontier.empty()) {
    Pos cur = frontier.front();
    frontier.pop();
    if (cur == to) return true;
    for (Dir dir : dirs) {
      Pos next = step(cur, dir);
      if (!d.inBounds(next) || !d.tile(next).isWalkable()) continue;
      size_t idx = static_cast<size_t>(next.y * d.width() + next.x);
      if (seen[idx]) continue;
      seen[idx] = 1;
      frontier.push(next);
    }
  }
  return false;
}

static bool sameDungeon(const Dungeon& a, const Dungeon& b) {
  if (a.width() != b.width() || a.height() != b.height()) return false;
  if (a.start() != b.start() || a.stairs() != b.stairs()) return false;
  for (int y = 0; y < a.height(); ++y) {
    for (int x = 0; x < a.width(); ++x) {
      if (a.tile({x, y}).type() != b.tile({x, y}).type()) return false;
    }
  }
  return true;
}

static bool borderIsWall(const Dungeon& d) {
  for (int x = 0; x < d.width(); ++x) {
    if (d.tile({x, 0}).type() != Wall) return false;
    if (d.tile({x, d.height() - 1}).type() != Wall) return false;
  }
  for (int y = 0; y < d.height(); ++y) {
    if (d.tile({0, y}).type() != Wall) return false;
    if (d.tile({d.width() - 1, y}).type() != Wall) return false;
  }
  return true;
}

static int countStairs(const Dungeon& d) {
  int n = 0;
  for (int y = 0; y < d.height(); ++y) {
    for (int x = 0; x < d.width(); ++x) {
      if (d.tile({x, y}).type() == Stairs) ++n;
    }
  }
  return n;
}

// Periksa semua sifat yang harus dimiliki peta hasil generate.
static void checkWellFormed(const Dungeon& d) {
  CHECK_EQ(d.tile(d.start()).type(), Floor);
  CHECK_EQ(d.tile(d.stairs()).type(), Stairs);
  CHECK(d.start() != d.stairs());
  CHECK_EQ(countStairs(d), 1);
  CHECK(borderIsWall(d));
  CHECK(reachable(d, d.start(), d.stairs()));
}

int main() {
  const DungeonGenerator generator;

  // --- Deterministik: seed sama = peta sama ---
  for (unsigned seed : {1u, 42u, 999u}) {
    Rng r1(seed);
    Rng r2(seed);
    Dungeon a = generator.generate(40, 25, r1);
    Dungeon b = generator.generate(40, 25, r2);
    CHECK(sameDungeon(a, b));
  }

  // --- Seed berbeda umumnya menghasilkan peta berbeda ---
  {
    Rng r1(1);
    Rng r2(2);
    Dungeon a = generator.generate(40, 25, r1);
    Dungeon b = generator.generate(40, 25, r2);
    CHECK(!sameDungeon(a, b));
  }

  // --- 40x25, seed 1..200: terhubung, tepi Wall, satu tangga ---
  for (unsigned seed = 1; seed <= 200; ++seed) {
    Rng rng(seed);
    Dungeon d = generator.generate(40, 25, rng);
    CHECK_EQ(d.width(), 40);
    CHECK_EQ(d.height(), 25);
    checkWellFormed(d);
  }

  // --- Ukuran minimum 12x10 juga harus valid ---
  for (unsigned seed = 1; seed <= 200; ++seed) {
    Rng rng(seed);
    Dungeon d = generator.generate(12, 10, rng);
    checkWellFormed(d);
  }

  // --- Ukuran terlalu kecil: std::invalid_argument ---
  {
    Rng rng(1);
    CHECK_THROWS(generator.generate(11, 10, rng), std::invalid_argument);
    CHECK_THROWS(generator.generate(12, 9, rng), std::invalid_argument);
    CHECK_THROWS(generator.generate(5, 5, rng), std::invalid_argument);
  }

  // --- Ruang loot: petak safe, lootCenter, dan variasi antar lantai ---
  {
    int withLoot = 0;
    int withoutLoot = 0;
    for (unsigned seed = 1; seed <= 300; ++seed) {
      Rng rng(seed);
      Dungeon d = generator.generate(40, 25, rng);
      int safeCount = 0;
      bool allFloor = true;
      for (int y = 0; y < d.height(); ++y) {
        for (int x = 0; x < d.width(); ++x) {
          if (!d.tile({x, y}).isSafe()) continue;
          ++safeCount;
          if (d.tile({x, y}).type() != Floor) allFloor = false;
        }
      }
      CHECK(allFloor);
      if (safeCount == 0) {
        ++withoutLoot;
        CHECK(!d.lootCenter().has_value());
        continue;
      }
      ++withLoot;
      CHECK(safeCount >= 12);  // ruang minimum 4x3
      CHECK(d.lootCenter().has_value());
      if (d.lootCenter().has_value()) CHECK(d.tile(*d.lootCenter()).isSafe());
      CHECK(!d.tile(d.start()).isSafe());
      CHECK(!d.tile(d.stairs()).isSafe());
      CHECK(reachable(d, d.start(), d.stairs()));
    }
    CHECK(withLoot > 0);
    CHECK(withoutLoot > 0);
  }

  // --- Peta kecil (12x10): bila ada ruang loot, tandanya konsisten ---
  for (unsigned seed = 1; seed <= 100; ++seed) {
    Rng rng(seed);
    Dungeon d = generator.generate(12, 10, rng);
    if (!d.lootCenter().has_value()) continue;
    CHECK(d.tile(*d.lootCenter()).isSafe());
    CHECK(d.tile(*d.lootCenter()).type() == Floor);
  }

  // --- Tata letak tidak berubah oleh penandaan safe (hash dari sebelum fitur) ---
  {
    std::uint64_t h = 1469598103934665603ULL;
    for (unsigned seed = 1; seed <= 50; ++seed) {
      Rng rng(seed);
      Dungeon d = generator.generate(40, 25, rng);
      for (int y = 0; y < 25; ++y) {
        for (int x = 0; x < 40; ++x) {
          h ^= static_cast<std::uint64_t>(d.tile({x, y}).type()) + 1;
          h *= 1099511628211ULL;
        }
      }
      h ^= static_cast<std::uint64_t>(d.start().x * 131 + d.start().y);
      h *= 1099511628211ULL;
      h ^= static_cast<std::uint64_t>(d.stairs().x * 137 + d.stairs().y);
      h *= 1099511628211ULL;
    }
    CHECK_EQ(h, static_cast<std::uint64_t>(416377646928606435ULL));
  }

  return check_summary();
}
