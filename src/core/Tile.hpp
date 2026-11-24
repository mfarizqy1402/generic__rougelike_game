#pragma once

namespace rogue {

enum TileType { Wall, Floor, Stairs };

// Satu petak peta. Hanya Wall yang tidak bisa dilewati.
class Tile {
 public:
  explicit Tile(TileType type = Wall);

  TileType type() const;
  void setType(TileType type);
  bool isWalkable() const;

  // Petak zona aman (ruang loot): musuh dilarang melangkah ke sini.
  bool isSafe() const;
  void setSafe(bool safe);

 private:
  TileType type_;
  bool safe_ = false;
};

}  // namespace rogue
