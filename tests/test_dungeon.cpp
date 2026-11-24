// Tes Tile dan Dungeon (geometri peta; query entity diuji di test_entities).
#include <stdexcept>
#include <utility>

#include "check.hpp"
#include "core/Dungeon.hpp"
#include "core/Exceptions.hpp"
#include "core/Tile.hpp"
#include "core/Types.hpp"

using namespace rogue;

int main() {
  // --- Tile: default Wall, isWalkable per jenis ---
  Tile defaultTile;
  CHECK_EQ(defaultTile.type(), Wall);
  CHECK(!defaultTile.isWalkable());

  Tile floorTile(Floor);
  CHECK(floorTile.isWalkable());

  Tile stairsTile(Stairs);
  CHECK(stairsTile.isWalkable());

  floorTile.setType(Wall);
  CHECK_EQ(floorTile.type(), Wall);
  CHECK(!floorTile.isWalkable());

  // --- Dungeon: ukuran dan inBounds ---
  Dungeon dungeon(12, 10);
  CHECK_EQ(dungeon.width(), 12);
  CHECK_EQ(dungeon.height(), 10);

  CHECK(dungeon.inBounds({0, 0}));
  CHECK(dungeon.inBounds({11, 9}));
  CHECK(!dungeon.inBounds({-1, 0}));
  CHECK(!dungeon.inBounds({0, -1}));
  CHECK(!dungeon.inBounds({12, 0}));
  CHECK(!dungeon.inBounds({0, 10}));

  // --- Semua petak awal adalah Wall dan tidak walkable ---
  bool allWall = true;
  for (int y = 0; y < dungeon.height(); ++y) {
    for (int x = 0; x < dungeon.width(); ++x) {
      if (dungeon.tile({x, y}).type() != Wall) allWall = false;
      if (dungeon.tile({x, y}).isWalkable()) allWall = false;
    }
  }
  CHECK(allWall);

  // --- Mengubah satu petak lewat referensi ---
  dungeon.tile({3, 4}).setType(Floor);
  CHECK_EQ(dungeon.tile({3, 4}).type(), Floor);
  CHECK(dungeon.tile({3, 4}).isWalkable());
  CHECK_EQ(dungeon.tile({4, 3}).type(), Wall);  // petak lain tidak ikut berubah

  // --- Versi const ---
  const Dungeon& view = dungeon;
  CHECK_EQ(view.tile({3, 4}).type(), Floor);

  // --- Di luar peta melempar InvalidMoveException (versi biasa dan const) ---
  CHECK_THROWS(dungeon.tile({-1, 0}), InvalidMoveException);
  CHECK_THROWS(dungeon.tile({12, 0}), InvalidMoveException);
  CHECK_THROWS(dungeon.tile({0, 10}), InvalidMoveException);
  CHECK_THROWS(view.tile({0, -1}), InvalidMoveException);
  CHECK_THROWS(view.tile({100, 100}), InvalidMoveException);

  // --- start dan stairs ---
  dungeon.setStart({2, 2});
  dungeon.setStairs({9, 7});
  CHECK_EQ(dungeon.start(), (Pos{2, 2}));
  CHECK_EQ(dungeon.stairs(), (Pos{9, 7}));

  // --- Ukuran tidak valid ---
  CHECK_THROWS(Dungeon(0, 5), std::invalid_argument);
  CHECK_THROWS(Dungeon(5, -1), std::invalid_argument);

  // --- Dungeon bisa dipindahkan (move), isinya ikut ---
  Dungeon moved(std::move(dungeon));
  CHECK_EQ(moved.width(), 12);
  CHECK_EQ(moved.tile({3, 4}).type(), Floor);
  CHECK_EQ(moved.stairs(), (Pos{9, 7}));

  // --- Tile.safe dan Dungeon.lootCenter ---
  Dungeon loot(12, 10);
  CHECK(!loot.tile({3, 3}).isSafe());
  loot.tile({3, 3}).setSafe(true);
  CHECK(loot.tile({3, 3}).isSafe());
  CHECK(!loot.tile({3, 4}).isSafe());
  CHECK(!loot.lootCenter().has_value());
  loot.setLootCenter({4, 4});
  CHECK(loot.lootCenter().has_value());
  CHECK(loot.lootCenter().has_value() && *loot.lootCenter() == (Pos{4, 4}));
  Dungeon lootMoved(std::move(loot));
  CHECK(lootMoved.lootCenter().has_value());
  CHECK(lootMoved.tile({3, 3}).isSafe());

  return check_summary();
}
