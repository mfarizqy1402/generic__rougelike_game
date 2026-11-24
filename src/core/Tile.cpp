#include "core/Tile.hpp"

namespace rogue {

Tile::Tile(TileType type) : type_(type) {}

TileType Tile::type() const { return type_; }

void Tile::setType(TileType type) { type_ = type; }

bool Tile::isWalkable() const { return type_ != Wall; }

bool Tile::isSafe() const { return safe_; }

void Tile::setSafe(bool safe) { safe_ = safe; }

}  // namespace rogue
