#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "core/Tile.hpp"
#include "core/Types.hpp"

namespace rogue {

class Entity;

// Peta satu lantai: grid Tile, posisi awal, posisi tangga, dan daftar entity
// yang dimiliki (composition: entity ikut hancur bersama Dungeon).
class Dungeon {
 public:
  Dungeon(int width, int height);
  ~Dungeon();

  // Tidak bisa disalin, tetapi bisa dipindahkan.
  Dungeon(const Dungeon&) = delete;
  Dungeon& operator=(const Dungeon&) = delete;
  Dungeon(Dungeon&&) noexcept;
  Dungeon& operator=(Dungeon&&) noexcept;

  int width() const;
  int height() const;
  bool inBounds(Pos p) const;

  // Di luar peta melempar InvalidMoveException.
  Tile& tile(Pos p);
  const Tile& tile(Pos p) const;

  Pos start() const;
  void setStart(Pos p);
  Pos stairs() const;
  void setStairs(Pos p);

  // Titik tengah ruang loot (zona aman); kosong bila lantai tanpa ruang loot.
  void setLootCenter(Pos p);
  std::optional<Pos> lootCenter() const;

  // Dungeon mengambil alih kepemilikan entity; mengembalikan referensinya.
  Entity& add(std::unique_ptr<Entity> entity);

  // Hanya entity yang masih hidup. nullptr bila petak kosong.
  Entity* entityAt(Pos p);
  const Entity* entityAt(Pos p) const;

  // Entity pertama yang bertipe Player (dynamic_cast), atau nullptr.
  Entity* player();
  const Entity* player() const;

  // Termasuk entity yang sudah mati (penghapusan diurus pemanggil).
  std::vector<std::unique_ptr<Entity>>& entities();
  const std::vector<std::unique_ptr<Entity>>& entities() const;

 private:
  std::size_t indexOf(Pos p) const;

  int width_;
  int height_;
  std::vector<Tile> tiles_;
  Pos start_;
  Pos stairs_;
  std::optional<Pos> lootCenter_;
  std::vector<std::unique_ptr<Entity>> entities_;
};

}  // namespace rogue
