#pragma once

#include <string>

#include "core/Rng.hpp"
#include "core/Types.hpp"

namespace rogue {

class Entity;

// Kelas dasar ABSTRAK untuk efek status sementara (cegukan, dsb.).
// Setiap efek berlaku selama sejumlah giliran lalu kedaluwarsa.
class StatusEffect {
 public:
  explicit StatusEffect(int turns);
  virtual ~StatusEffect() = default;

  virtual std::string name() const = 0;

  // Dipanggil di awal giliran pemilik. Bawaan: tidak melakukan apa pun.
  virtual void onTurnStart(Entity& owner);

  // Boleh mengubah arah gerak yang diminta. Dir::None berarti diam.
  virtual Dir modifyMove(Dir requested, Rng& rng) = 0;

  bool expired() const;
  void tick();  // mengurangi sisa giliran (tidak di bawah 0)

 private:
  int turnsLeft_;
};

}  // namespace rogue
