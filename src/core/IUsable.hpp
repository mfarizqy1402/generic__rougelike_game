#pragma once

#include <string>

namespace rogue {

class Player;

// INTERFACE: sesuatu yang bisa dipakai pemain. Mengembalikan pesan hasilnya.
class IUsable {
 public:
  virtual ~IUsable() = default;
  virtual std::string use(Player& player) = 0;
};

}  // namespace rogue
