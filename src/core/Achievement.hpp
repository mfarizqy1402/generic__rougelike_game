#pragma once

#include <string>

#include "core/GameEvent.hpp"
#include "core/IGameObserver.hpp"

namespace rogue {

// Kelas dasar ABSTRAK untuk achievement. Achievement adalah pengamat game:
// onEvent() bersifat final (pola Template Method), sehingga aturan reset
// berlaku sama untuk semua achievement:
//   RunStarted  -> unlocked = false, lalu reset() milik subkelas
//   event lain  -> handle() milik subkelas
class Achievement : public IGameObserver {
 public:
  explicit Achievement(std::string title);

  const std::string& title() const;
  bool unlocked() const;

  void onEvent(const GameEvent& event) final;

 protected:
  // Dipanggil subkelas saat syarat terpenuhi.
  void unlock();

  virtual void handle(const GameEvent& event) = 0;
  virtual void reset() = 0;

 private:
  std::string title_;
  bool unlocked_ = false;
};

}  // namespace rogue
