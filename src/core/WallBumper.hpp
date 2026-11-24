#pragma once

#include "core/Achievement.hpp"

namespace rogue {

// "Menabrak tembok 50 kali": terbuka setelah 50 event WallBump dalam satu run.
class WallBumper : public Achievement {
 public:
  static constexpr int kRequired = 50;

  WallBumper();

 protected:
  void handle(const GameEvent& event) override;
  void reset() override;

 private:
  int bumps_ = 0;
};

}  // namespace rogue
