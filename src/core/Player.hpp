#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/Entity.hpp"

namespace rogue {

class Item;

// Pemain. Gerakannya diatur GameController, jadi act() kosong.
class Player : public Entity {
 public:
  static constexpr int kMaxHp = 20;

  Player(std::string name, Pos pos);

  void act(Dungeon& dungeon, Rng& rng) override;

  // Menambah hp, dibatasi kMaxHp. Nilai negatif diabaikan.
  void heal(int amount);

  std::vector<std::shared_ptr<Item>>& inventory();
  const std::vector<std::shared_ptr<Item>>& inventory() const;

  int depth() const;
  void setDepth(int depth);

 private:
  std::vector<std::shared_ptr<Item>> inventory_;
  int depth_ = 1;
};

}  // namespace rogue
