#pragma once

#include <string>

#include "core/Item.hpp"

namespace rogue {

// Ramuan: memulihkan 8 hp (dibatasi hp maksimum pemain).
class Potion : public Item {
 public:
  static constexpr int kHealAmount = 8;

  Potion();

  std::string use(Player& player) override;
};

}  // namespace rogue
