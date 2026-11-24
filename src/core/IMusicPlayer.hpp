#pragma once

#include <string>

namespace rogue {

// Antarmuka pemutar musik latar. Game hanya bergantung pada antarmuka ini,
// bukan pada library audio tertentu (Dependency Inversion).
class IMusicPlayer {
 public:
  virtual ~IMusicPlayer() = default;

  virtual void start() = 0;
  virtual void stop() = 0;  // aman dipanggil berulang
  // Nama berkas yang sedang diputar (tanpa folder), atau "" bila tidak ada.
  virtual std::string nowPlaying() const = 0;
};

}  // namespace rogue
