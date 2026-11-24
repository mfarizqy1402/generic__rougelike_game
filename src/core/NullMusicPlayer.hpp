#pragma once

#include <string>

#include "core/IMusicPlayer.hpp"

namespace rogue {

// Null Object: pemutar yang tidak melakukan apa pun. Dipakai bila folder
// music/ kosong, sehingga kode pemanggil tidak perlu memeriksa nullptr.
class NullMusicPlayer : public IMusicPlayer {
 public:
  void start() override {}
  void stop() override {}
  std::string nowPlaying() const override { return ""; }
};

}  // namespace rogue
