#pragma once

#include <memory>
#include <string>

#include "core/IMusicPlayer.hpp"
#include "core/ShuffleQueue.hpp"

namespace rogue {

struct MiniaudioOptions {
  // true = backend "null" miniaudio (tanpa perangkat suara; dipakai tes).
  bool useNullBackend = false;
};

// Pemutar BGM berbasis miniaudio. Satu thread pengelola memutar lagu dari
// ShuffleQueue satu per satu. Kegagalan apa pun (tidak ada perangkat suara,
// berkas rusak) membuat pemutar diam, tidak pernah melempar atau menggantung.
// Sekali stop() dipanggil, pemutar tidak dapat dimulai ulang.
class MiniaudioPlayer : public IMusicPlayer {
 public:
  explicit MiniaudioPlayer(ShuffleQueue queue, MiniaudioOptions options = {});
  ~MiniaudioPlayer() override;
  MiniaudioPlayer(const MiniaudioPlayer&) = delete;
  MiniaudioPlayer& operator=(const MiniaudioPlayer&) = delete;

  void start() override;
  void stop() override;
  std::string nowPlaying() const override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace rogue
