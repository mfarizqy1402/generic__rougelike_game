#include "ui/Viewport.hpp"

#include <algorithm>
#include <stdexcept>

#include "core/Chest.hpp"
#include "core/Dungeon.hpp"
#include "core/Entity.hpp"
#include "core/Ghost.hpp"
#include "core/Goblin.hpp"
#include "core/PetRock.hpp"
#include "core/Player.hpp"
#include "core/Slime.hpp"
#include "core/Tile.hpp"

namespace rogue {

namespace {

// Pojok awal viewport pada satu sumbu: berpusat pada pemain, lalu dijepit.
int clampedOrigin(int playerCoord, int viewSize, int mapSize) {
  int origin = playerCoord - viewSize / 2;
  origin = std::min(origin, mapSize - viewSize);
  origin = std::max(origin, 0);
  return origin;
}

Cell cellForTile(const Tile& tile) {
  switch (tile.type()) {
    case Wall:
      return Cell{'#', ColorId::Dim};
    case Floor:
      // Lantai dibiarkan kosong; hanya zona aman yang diberi warna (latar hijau).
      return Cell{' ', tile.isSafe() ? ColorId::DarkGreen : ColorId::Default};
    case Stairs:
      return Cell{'>', ColorId::Yellow};
  }
  return Cell{'?', ColorId::Default};
}

Cell cellForEntity(const Entity& entity) {
  if (dynamic_cast<const Player*>(&entity) != nullptr) {
    return Cell{'@', ColorId::Cyan};
  }
  if (dynamic_cast<const Goblin*>(&entity) != nullptr) {
    return Cell{'g', ColorId::Red};
  }
  if (dynamic_cast<const Slime*>(&entity) != nullptr) {
    return Cell{'s', ColorId::Red};
  }
  if (dynamic_cast<const Ghost*>(&entity) != nullptr) {
    return Cell{'H', ColorId::Red};
  }
  if (dynamic_cast<const PetRock*>(&entity) != nullptr) {
    return Cell{'o', ColorId::Brown};
  }
  if (dynamic_cast<const Chest*>(&entity) != nullptr) {
    return Cell{'C', ColorId::LightGreen};
  }
  return Cell{'?', ColorId::Default};
}

}  // namespace

std::vector<std::vector<Cell>> buildViewport(const GameController& controller,
                                             int viewW, int viewH) {
  if (viewW <= 0 || viewH <= 0) {
    throw std::invalid_argument("Ukuran viewport harus lebih besar dari nol.");
  }

  const Dungeon& dungeon = controller.dungeon();  // melempar bila belum mulai
  const Pos center = controller.playerPos();
  const int originX = clampedOrigin(center.x, viewW, dungeon.width());
  const int originY = clampedOrigin(center.y, viewH, dungeon.height());

  std::vector<std::vector<Cell>> grid(
      static_cast<std::size_t>(viewH),
      std::vector<Cell>(static_cast<std::size_t>(viewW), Cell{}));

  // Lapisan 1: petak peta.
  for (int y = 0; y < viewH; ++y) {
    for (int x = 0; x < viewW; ++x) {
      const Pos world{originX + x, originY + y};
      if (!dungeon.inBounds(world)) continue;
      grid[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] =
          cellForTile(dungeon.tile(world));
    }
  }

  // Lapisan 2: entity hidup. Pemain digambar terakhir supaya selalu terlihat.
  auto draw = [&](const Entity& entity) {
    const int x = entity.pos().x - originX;
    const int y = entity.pos().y - originY;
    if (x < 0 || y < 0 || x >= viewW || y >= viewH) return;
    grid[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] =
        cellForEntity(entity);
  };
  for (const auto& entity : dungeon.entities()) {
    if (!entity->isAlive()) continue;
    if (dynamic_cast<const Player*>(entity.get()) != nullptr) continue;
    draw(*entity);
  }
  for (const auto& entity : dungeon.entities()) {
    if (entity->isAlive() && dynamic_cast<const Player*>(entity.get()) != nullptr) {
      draw(*entity);
    }
  }

  return grid;
}

std::vector<LegendItem> legend() {
  return {
      {'@', ColorId::Cyan, "Kamu", false},
      {'g', ColorId::Red, "Goblin", true},
      {'s', ColorId::Red, "Slime", true},
      {'H', ColorId::Red, "Hantu", true},
      {'o', ColorId::Brown, "Batu", false},
      {'C', ColorId::LightGreen, "Peti", false},
      {'>', ColorId::Yellow, "Tangga", false},
      {' ', ColorId::DarkGreen, "Zona aman", false},
      {'#', ColorId::Dim, "Tembok", false},
  };
}

}  // namespace rogue
