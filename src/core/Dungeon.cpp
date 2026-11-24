#include "core/Dungeon.hpp"

#include <stdexcept>
#include <utility>

#include "core/Entity.hpp"
#include "core/Exceptions.hpp"
#include "core/Player.hpp"

namespace rogue {

Dungeon::Dungeon(int width, int height) : width_(width), height_(height) {
  if (width <= 0 || height <= 0) {
    throw std::invalid_argument("Ukuran peta harus lebih besar dari nol.");
  }
  tiles_.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
}

// Destruktor dan move didefinisikan di .cpp agar tipe Entity yang lengkap
// cukup dibutuhkan di sini, bukan di header.
Dungeon::~Dungeon() = default;
Dungeon::Dungeon(Dungeon&&) noexcept = default;
Dungeon& Dungeon::operator=(Dungeon&&) noexcept = default;

void Dungeon::setLootCenter(Pos p) { lootCenter_ = p; }

std::optional<Pos> Dungeon::lootCenter() const { return lootCenter_; }

int Dungeon::width() const { return width_; }
int Dungeon::height() const { return height_; }

bool Dungeon::inBounds(Pos p) const {
  return p.x >= 0 && p.y >= 0 && p.x < width_ && p.y < height_;
}

size_t Dungeon::indexOf(Pos p) const {
  if (!inBounds(p)) {
    throw InvalidMoveException("Petak berada di luar peta.");
  }
  return static_cast<size_t>(p.y) * static_cast<size_t>(width_) +
         static_cast<size_t>(p.x);
}

Tile& Dungeon::tile(Pos p) { return tiles_[indexOf(p)]; }
const Tile& Dungeon::tile(Pos p) const { return tiles_[indexOf(p)]; }

Pos Dungeon::start() const { return start_; }
void Dungeon::setStart(Pos p) { start_ = p; }
Pos Dungeon::stairs() const { return stairs_; }
void Dungeon::setStairs(Pos p) { stairs_ = p; }

Entity& Dungeon::add(std::unique_ptr<Entity> entity) {
  if (!entity) {
    throw std::invalid_argument("Entity tidak boleh kosong.");
  }
  entities_.push_back(std::move(entity));
  return *entities_.back();
}

const Entity* Dungeon::entityAt(Pos p) const {
  for (const auto& entity : entities_) {
    if (entity->isAlive() && entity->pos() == p) return entity.get();
  }
  return nullptr;
}

Entity* Dungeon::entityAt(Pos p) {
  return const_cast<Entity*>(std::as_const(*this).entityAt(p));
}

const Entity* Dungeon::player() const {
  for (const auto& entity : entities_) {
    if (dynamic_cast<const Player*>(entity.get()) != nullptr) {
      return entity.get();
    }
  }
  return nullptr;
}

Entity* Dungeon::player() {
  return const_cast<Entity*>(std::as_const(*this).player());
}

std::vector<std::unique_ptr<Entity>>& Dungeon::entities() { return entities_; }
const std::vector<std::unique_ptr<Entity>>& Dungeon::entities() const {
  return entities_;
}

}  // namespace rogue
