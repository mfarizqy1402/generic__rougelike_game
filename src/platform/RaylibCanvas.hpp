#pragma once

#include <string>

#include "ui/ICanvas.hpp"

namespace rogue {

// Implementasi ICanvas memakai raylib. Hanya boleh dipanggil di antara
// BeginDrawing() dan EndDrawing().
class RaylibCanvas : public ICanvas {
 public:
  void fillRect(int x, int y, int w, int h, Rgb color) override;
  void strokeRect(int x, int y, int w, int h, Rgb color) override;
  void gradientRect(int x, int y, int w, int h, Rgb top, Rgb bottom) override;
  void bevelRect(int x, int y, int w, int h, Rgb face, Rgb light, Rgb dark) override;
  void text(const std::string& s, int x, int y, int size, Rgb color) override;
  int textWidth(const std::string& s, int size) const override;
};

}  // namespace rogue
