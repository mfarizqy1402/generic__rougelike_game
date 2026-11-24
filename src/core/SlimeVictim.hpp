#pragma once

#include "core/Achievement.hpp"

namespace rogue {

// "Mati karena slime": terbuka bila pemain tewas oleh Slime.
class SlimeVictim : public Achievement {
 public:
  SlimeVictim();

 protected:
  void handle(const GameEvent& event) override;
  void reset() override;
};

}  // namespace rogue
