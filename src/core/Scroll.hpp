#pragma once

#include <string>

#include "core/Item.hpp"

namespace rogue {

// Gulungan Konyol: membuat pemain cegukan selama 5 giliran.
class Scroll : public Item {
 public:
  static constexpr int kHiccupTurns = 5;

  Scroll();

  std::string use(Player& player) override;
};

}  // namespace rogue
