#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/Rng.hpp"
#include "core/StatusEffect.hpp"
#include "core/Types.hpp"

namespace rogue {

class Dungeon;

// Kelas dasar ABSTRAK untuk semua makhluk di peta (pemain, musuh, Pet Rock).
// Subkelas wajib mengimplementasikan act().
class Entity {
 public:
  Entity(std::string name, int hp, Pos pos);
  virtual ~Entity();

  Entity(const Entity&) = delete;
  Entity& operator=(const Entity&) = delete;

  // Giliran entity ini. Polimorfik: tiap subkelas berperilaku berbeda.
  virtual void act(Dungeon& dungeon, Rng& rng) = 0;

  // Ejekan khas; bawaan kosong.
  virtual std::string taunt() const;

  // Mengurangi hp (tidak di bawah 0). Damage negatif diabaikan.
  virtual void takeDamage(int amount);

  const std::string& name() const;
  int hp() const;
  bool isAlive() const;
  Pos pos() const;
  void setPos(Pos pos);

  void addStatus(std::unique_ptr<StatusEffect> status);
  std::vector<std::unique_ptr<StatusEffect>>& statuses();
  const std::vector<std::unique_ptr<StatusEffect>>& statuses() const;

 protected:
  void setHp(int hp);

 private:
  std::string name_;
  int hp_;
  Pos pos_;
  std::vector<std::unique_ptr<StatusEffect>> statuses_;
};

}  // namespace rogue
