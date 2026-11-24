#pragma once

#include "core/Achievement.hpp"

namespace rogue {

// "Menunggu 20 kali tanpa alasan": terbuka setelah 20 event Waited dengan
// detail "diam" (diam atas kemauan sendiri, bukan karena tertahan status).
class Pacifist : public Achievement {
 public:
  static constexpr int kRequired = 20;

  Pacifist();

 protected:
  void handle(const GameEvent& event) override;
  void reset() override;

 private:
  int waits_ = 0;
};

}  // namespace rogue
