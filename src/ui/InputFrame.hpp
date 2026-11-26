#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace rogue {

enum class Key { Up, Down, Left, Right, Enter, Escape, Tab, Backspace };

// Masukan satu frame: tombol yang baru ditekan, karakter ASCII (32..126) yang
// diketik, dan klik mouse kiri. Huruf, spasi, dan angka datang lewat `typed`.
struct InputFrame {
  std::vector<Key> pressed;
  std::string typed;
  bool clicked = false;
  int mouseX = 0;
  int mouseY = 0;

  bool pressedKey(Key key) const {
    return std::find(pressed.begin(), pressed.end(), key) != pressed.end();
  }
};

}  // namespace rogue
