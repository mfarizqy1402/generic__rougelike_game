#pragma once

#include <string>

#include "core/Entity.hpp"

namespace rogue {

// Kelas dasar ABSTRAK untuk musuh. act() tetap abstrak: tiap musuh memutuskan
// kapan dan bagaimana memanggil chase().
class Enemy : public Entity {
 public:
  Enemy(std::string name, int hp, Pos pos, int damage);

  int damage() const;

  // True bila pemanggilan act() terakhir berupa serangan ke pemain.
  bool attackedLastTurn() const;

 protected:
  // Menempel pemain (jarak Manhattan 1): serang. Selain itu: satu langkah
  // mendekat. Tanpa pemain hidup: tidak melakukan apa pun.
  void chase(Dungeon& dungeon, bool throughWalls);

  // Dipanggil subkelas pada giliran yang tidak mengejar (mis. Slime),
  // supaya bendera serangan giliran sebelumnya padam.
  void clearAttack();

 private:
  int damage_;
  bool attacked_ = false;
};

}  // namespace rogue
