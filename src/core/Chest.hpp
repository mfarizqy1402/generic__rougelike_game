#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/Entity.hpp"
#include "core/Item.hpp"

namespace rogue {

// Peti di ruang loot: entity pasif dengan 1 HP yang membawa item. Pemain
// "membukanya" dengan menyerang; isinya diambil GameController saat peti pecah.
class Chest : public Entity {
 public:
  static constexpr int kHp = 1;

  Chest(Pos pos, std::vector<std::shared_ptr<Item>> loot);

  // Peti tidak bertindak.
  void act(Dungeon& dungeon, Rng& rng) override;

  const std::vector<std::shared_ptr<Item>>& loot() const;

  // Menyerahkan seluruh isi dan mengosongkan peti.
  std::vector<std::shared_ptr<Item>> takeLoot();

 private:
  std::vector<std::shared_ptr<Item>> loot_;
};

}  // namespace rogue
