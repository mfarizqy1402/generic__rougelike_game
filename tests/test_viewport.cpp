// Tes ui/Viewport (logika tampilan tanpa UI).
//
// Seperti test_controller, tes ini memakai const_cast pada controller.dungeon()
// agar situasinya pasti (posisi pemain, entity di sekitarnya). Hanya untuk tes.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "check.hpp"
#include "controller/GameController.hpp"
#include "core/Chest.hpp"
#include "core/Dungeon.hpp"
#include "core/Exceptions.hpp"
#include "core/Ghost.hpp"
#include "core/Goblin.hpp"
#include "core/PetRock.hpp"
#include "core/Potion.hpp"
#include "core/Player.hpp"
#include "core/Slime.hpp"
#include "core/Tile.hpp"
#include "core/Types.hpp"
#include "ui/Viewport.hpp"

using namespace rogue;

namespace {

constexpr int kViewW = 25;
constexpr int kViewH = 15;

Dungeon& mutableDungeon(GameController& c) {
  return const_cast<Dungeon&>(c.dungeon());
}

Player& playerOf(GameController& c) {
  return *dynamic_cast<Player*>(mutableDungeon(c).player());
}

// Menyisakan hanya pemain di peta.
void removeAllButPlayer(GameController& c) {
  auto& all = mutableDungeon(c).entities();
  all.erase(std::remove_if(all.begin(), all.end(),
                           [](const std::unique_ptr<Entity>& e) {
                             return dynamic_cast<Player*>(e.get()) == nullptr;
                           }),
            all.end());
}

// Aturan penjepitan, ditulis ulang di sini sebagai spesifikasi: viewport
// berpusat pada pemain, tetapi tidak boleh keluar dari tepi peta.
int expectedOrigin(int playerCoord, int viewSize, int mapSize) {
  int origin = playerCoord - viewSize / 2;
  if (origin > mapSize - viewSize) origin = mapSize - viewSize;
  if (origin < 0) origin = 0;
  return origin;
}

struct Found {
  int x = -1;
  int y = -1;
  int count = 0;
};

Found findGlyph(const std::vector<std::vector<Cell>>& grid, char glyph) {
  Found found;
  for (std::size_t y = 0; y < grid.size(); ++y) {
    for (std::size_t x = 0; x < grid[y].size(); ++x) {
      if (grid[y][x].glyph == glyph) {
        found.x = static_cast<int>(x);
        found.y = static_cast<int>(y);
        ++found.count;
      }
    }
  }
  return found;
}

const Cell& cellAt(const std::vector<std::vector<Cell>>& grid, int x, int y) {
  return grid[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
}

void testSizeMatchesRequest() {
  GameController c;
  c.startRun("Budi", "1", Difficulty::Normal);

  const int sizes[4][2] = {{kViewW, kViewH}, {10, 6}, {40, 25}, {50, 30}};
  for (const auto& size : sizes) {
    const auto grid = buildViewport(c, size[0], size[1]);
    CHECK_EQ(grid.size(), static_cast<std::size_t>(size[1]));
    bool allRowsRight = true;
    for (const auto& row : grid) {
      if (row.size() != static_cast<std::size_t>(size[0])) allRowsRight = false;
    }
    CHECK(allRowsRight);
  }
}

void testPlayerIsAlwaysInViewport() {
  for (std::uint32_t seed = 1; seed <= 20; ++seed) {
    GameController c;
    c.startRun("Budi", std::to_string(seed), Difficulty::Normal);
    const auto grid = buildViewport(c, kViewW, kViewH);

    const Found at = findGlyph(grid, '@');
    CHECK_EQ(at.count, 1);

    const Pos p = c.playerPos();
    const int ox = expectedOrigin(p.x, kViewW, c.dungeon().width());
    const int oy = expectedOrigin(p.y, kViewH, c.dungeon().height());
    CHECK_EQ(at.x, p.x - ox);
    CHECK_EQ(at.y, p.y - oy);
    if (at.count == 1) CHECK(cellAt(grid, at.x, at.y).color == ColorId::Cyan);
  }
}

void testViewportCentersAndClampsAtMapEdges() {
  GameController c;
  c.startRun("Budi", "2", Difficulty::Normal);
  removeAllButPlayer(c);
  Player& player = playerOf(c);

  struct Case {
    Pos pos;
    Pos expectedCell;
  };
  const Case cases[] = {
      {Pos{20, 12}, Pos{12, 7}},   // di tengah: pemain tepat di pusat viewport
      {Pos{1, 1}, Pos{1, 1}},      // pojok kiri atas: dijepit
      {Pos{0, 0}, Pos{0, 0}},
      {Pos{38, 23}, Pos{23, 13}},  // pojok kanan bawah: dijepit
      {Pos{39, 24}, Pos{24, 14}},
      {Pos{5, 12}, Pos{5, 7}},     // dekat tepi kiri saja
      {Pos{35, 12}, Pos{20, 7}},   // dekat tepi kanan saja
      {Pos{20, 2}, Pos{12, 2}},    // dekat tepi atas saja
      {Pos{20, 22}, Pos{12, 12}},  // dekat tepi bawah saja
  };
  for (const Case& test : cases) {
    player.setPos(test.pos);
    const auto grid = buildViewport(c, kViewW, kViewH);
    const Found at = findGlyph(grid, '@');
    CHECK_EQ(at.count, 1);
    CHECK_EQ(at.x, test.expectedCell.x);
    CHECK_EQ(at.y, test.expectedCell.y);
  }
}

void testTilesAreDrawnWithGlyphsAndColors() {
  GameController c;
  c.startRun("Budi", "3", Difficulty::Normal);
  removeAllButPlayer(c);
  const Dungeon& d = c.dungeon();
  const Pos p = c.playerPos();

  const auto grid = buildViewport(c, kViewW, kViewH);
  const int ox = expectedOrigin(p.x, kViewW, d.width());
  const int oy = expectedOrigin(p.y, kViewH, d.height());

  bool tilesMatch = true;
  bool sawWall = false;
  bool sawFloor = false;
  bool sawSafeFloor = false;
  for (int y = 0; y < kViewH; ++y) {
    for (int x = 0; x < kViewW; ++x) {
      const Pos world{ox + x, oy + y};
      const Cell& cell = cellAt(grid, x, y);
      if (world == p) continue;  // pemain menimpa petaknya
      switch (d.tile(world).type()) {
        case Wall:
          sawWall = true;
          if (cell.glyph != '#' || cell.color != ColorId::Dim) tilesMatch = false;
          break;
        case Floor:
          sawFloor = true;
          if (d.tile(world).isSafe()) {
            sawSafeFloor = true;
            if (cell.glyph != ' ' || cell.color != ColorId::DarkGreen) tilesMatch = false;
          } else if (cell.glyph != ' ' || cell.color != ColorId::Default) {
            tilesMatch = false;
          }
          break;
        case Stairs:
          if (cell.glyph != '>' || cell.color != ColorId::Yellow) tilesMatch = false;
          break;
      }
    }
  }
  CHECK(tilesMatch);
  CHECK(sawWall);
  CHECK(sawFloor);
  (void)sawSafeFloor;  // petak safe diuji khusus di testChestGlyphAndSafeFloorColor
}

void testStairsGlyphIsYellow() {
  GameController c;
  c.startRun("Budi", "4", Difficulty::Normal);
  removeAllButPlayer(c);
  const Pos stairs = c.dungeon().stairs();
  // Pemain berdiri di sebelah tangga supaya tangga ikut terlihat.
  playerOf(c).setPos(Pos{stairs.x + 1, stairs.y});

  const auto grid = buildViewport(c, kViewW, kViewH);
  const Pos p = c.playerPos();
  const int ox = expectedOrigin(p.x, kViewW, c.dungeon().width());
  const int oy = expectedOrigin(p.y, kViewH, c.dungeon().height());
  const Cell& cell = cellAt(grid, stairs.x - ox, stairs.y - oy);
  CHECK_EQ(cell.glyph, '>');
  CHECK(cell.color == ColorId::Yellow);
}

void testEntityGlyphsAndColors() {
  GameController c;
  c.startRun("Budi", "5", Difficulty::Normal);
  removeAllButPlayer(c);
  Dungeon& d = mutableDungeon(c);
  playerOf(c).setPos(Pos{20, 12});  // di tengah: viewport mulai di (8, 5)

  d.add(std::make_unique<Goblin>(Pos{21, 12}));
  d.add(std::make_unique<Slime>(Pos{22, 12}));
  d.add(std::make_unique<Ghost>(Pos{23, 12}));
  d.add(std::make_unique<PetRock>(Pos{19, 12}));

  const auto grid = buildViewport(c, kViewW, kViewH);
  // Kolom viewport = posisi dunia - 8; baris = 12 - 5 = 7.
  const Cell& goblin = cellAt(grid, 21 - 8, 7);
  const Cell& slime = cellAt(grid, 22 - 8, 7);
  const Cell& ghost = cellAt(grid, 23 - 8, 7);
  const Cell& rock = cellAt(grid, 19 - 8, 7);
  const Cell& hero = cellAt(grid, 20 - 8, 7);

  CHECK_EQ(goblin.glyph, 'g');
  CHECK(goblin.color == ColorId::Red);
  CHECK_EQ(slime.glyph, 's');
  CHECK(slime.color == ColorId::Red);
  CHECK_EQ(ghost.glyph, 'H');
  CHECK(ghost.color == ColorId::Red);
  CHECK_EQ(rock.glyph, 'o');
  CHECK(rock.color == ColorId::Brown);
  CHECK_EQ(hero.glyph, '@');
  CHECK(hero.color == ColorId::Cyan);
}

void testNoDotsAnywhereInViewport() {
  for (unsigned seed = 1; seed <= 20; ++seed) {
    GameController c;
    c.startRun("Budi", std::to_string(seed), Difficulty::Normal);
    for (const auto& row : buildViewport(c, 40, 25)) {
      for (const Cell& cell : row) CHECK(cell.glyph != '.');
    }
  }
}

void testChestGlyphAndSafeFloorColor() {
  GameController c;
  c.startRun("Budi", "5", Difficulty::Normal);
  removeAllButPlayer(c);
  Dungeon& d = mutableDungeon(c);
  playerOf(c).setPos(Pos{20, 12});
  d.add(std::make_unique<Chest>(
      Pos{21, 12}, std::vector<std::shared_ptr<Item>>{std::make_shared<Potion>()}));
  d.tile(Pos{22, 12}).setType(Floor);
  d.tile(Pos{22, 12}).setSafe(true);
  d.tile(Pos{23, 12}).setType(Floor);

  const auto grid = buildViewport(c, kViewW, kViewH);
  const Cell& chest = cellAt(grid, 21 - 8, 7);
  CHECK_EQ(chest.glyph, 'C');
  CHECK(chest.color == ColorId::LightGreen);
  const Cell& safe = cellAt(grid, 22 - 8, 7);
  CHECK_EQ(safe.glyph, ' ');
  CHECK(safe.color == ColorId::DarkGreen);
  const Cell& normal = cellAt(grid, 23 - 8, 7);
  CHECK_EQ(normal.glyph, ' ');  // lantai biasa kosong, tanpa titik
  CHECK(normal.color == ColorId::Default);
}

void testLegend() {
  const std::vector<LegendItem> items = legend();
  CHECK(!items.empty());
  int enemies = 0;
  for (const LegendItem& item : items) {
    CHECK(!item.label.empty());
    CHECK(item.glyph != ' ' || item.color == ColorId::DarkGreen);  // zona aman: kotak latar
    if (item.enemy) {
      ++enemies;
      CHECK(item.color == ColorId::Red);
    } else {
      CHECK(item.color != ColorId::Red);
    }
  }
  CHECK_EQ(enemies, 3);  // Goblin, Slime, Hantu

  auto has = [&](char glyph, ColorId color, const std::string& label) {
    for (const LegendItem& item : items) {
      if (item.glyph == glyph && item.color == color && item.label == label) return true;
    }
    return false;
  };
  CHECK(has('@', ColorId::Cyan, "Kamu"));
  CHECK(has('g', ColorId::Red, "Goblin"));
  CHECK(has('s', ColorId::Red, "Slime"));
  CHECK(has('H', ColorId::Red, "Hantu"));
  CHECK(has('o', ColorId::Brown, "Batu"));
  CHECK(has('C', ColorId::LightGreen, "Peti"));
  CHECK(has('>', ColorId::Yellow, "Tangga"));
  CHECK(has(' ', ColorId::DarkGreen, "Zona aman"));
  CHECK(has('#', ColorId::Dim, "Tembok"));
}

// Setiap simbol berwarna yang bisa tampil di peta (selain dinding dan lantai
// biasa) harus dijelaskan di legenda.
void testLegendCoversEverythingDrawn() {
  const std::vector<LegendItem> items = legend();
  for (unsigned seed = 1; seed <= 60; ++seed) {
    GameController c;
    c.startRun("Budi", std::to_string(seed), Difficulty::Sulit);
    const auto grid = buildViewport(c, 40, 25);
    for (const auto& row : grid) {
      for (const Cell& cell : row) {
        if (cell.glyph == ' ' && cell.color == ColorId::Default) continue;  // lantai kosong
        bool listed = false;
        for (const LegendItem& item : items) {
          if (item.glyph == cell.glyph && item.color == cell.color) listed = true;
        }
        CHECK(listed);
      }
    }
  }
}

void testDeadEntitiesAreNotDrawnAndPlayerIsOnTop() {
  GameController c;
  c.startRun("Budi", "6", Difficulty::Normal);
  removeAllButPlayer(c);
  Dungeon& d = mutableDungeon(c);
  playerOf(c).setPos(Pos{20, 12});

  Entity& dead = d.add(std::make_unique<Goblin>(Pos{21, 12}));
  dead.takeDamage(100);
  d.add(std::make_unique<Ghost>(Pos{20, 12}));  // Ghost bertumpuk dengan pemain

  const auto grid = buildViewport(c, kViewW, kViewH);
  CHECK_EQ(findGlyph(grid, 'g').count, 0);  // yang mati tidak digambar
  CHECK_EQ(cellAt(grid, 12, 7).glyph, '@');  // pemain paling atas
  CHECK_EQ(findGlyph(grid, 'H').count, 0);
}

void testViewportLargerThanMapIsPadded() {
  GameController c;
  c.startRun("Budi", "7", Difficulty::Normal);
  const auto grid = buildViewport(c, 50, 30);
  CHECK_EQ(cellAt(grid, 49, 29).glyph, ' ');  // di luar peta: kosong
  CHECK_EQ(cellAt(grid, 45, 0).glyph, ' ');
  CHECK_EQ(findGlyph(grid, '@').count, 1);
  // Peta 40x25 tampil utuh di pojok kiri atas.
  CHECK(cellAt(grid, 0, 0).glyph == '#');
  CHECK(cellAt(grid, 39, 24).glyph == '#');
}

void testInvalidArguments() {
  GameController c;
  CHECK_THROWS(buildViewport(c, kViewW, kViewH), InvalidMoveException);  // belum mulai

  c.startRun("Budi", "8", Difficulty::Normal);
  CHECK_THROWS(buildViewport(c, 0, kViewH), std::invalid_argument);
  CHECK_THROWS(buildViewport(c, kViewW, 0), std::invalid_argument);
  CHECK_THROWS(buildViewport(c, -3, 5), std::invalid_argument);
}

}  // namespace

int main() {
  testSizeMatchesRequest();
  testPlayerIsAlwaysInViewport();
  testViewportCentersAndClampsAtMapEdges();
  testTilesAreDrawnWithGlyphsAndColors();
  testStairsGlyphIsYellow();
  testEntityGlyphsAndColors();
  testNoDotsAnywhereInViewport();
  testChestGlyphAndSafeFloorColor();
  testLegend();
  testLegendCoversEverythingDrawn();
  testDeadEntitiesAreNotDrawnAndPlayerIsOnTop();
  testViewportLargerThanMapIsPadded();
  testInvalidArguments();
  return check_summary();
}
