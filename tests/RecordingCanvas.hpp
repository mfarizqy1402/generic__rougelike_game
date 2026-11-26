// Canvas palsu untuk tes layar: mencatat semua panggilan gambar.
#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "ui/ICanvas.hpp"

namespace rogue {

class RecordingCanvas : public ICanvas {
 public:
  enum class Kind { Fill, Stroke, Text, Gradient, Bevel };

  struct Call {
    Kind kind;
    int x, y, w, h;  // untuk Text: w = textWidth, h = size
    std::string text;
    Rgb color;   // Gradient: warna atas; Bevel: warna muka
    Rgb color2;  // Gradient: warna bawah; Bevel: warna sisi gelap
  };

  void fillRect(int x, int y, int w, int h, Rgb c) override {
    calls.push_back({Kind::Fill, x, y, w, h, "", c, c});
  }
  void strokeRect(int x, int y, int w, int h, Rgb c) override {
    calls.push_back({Kind::Stroke, x, y, w, h, "", c, c});
  }
  void text(const std::string& s, int x, int y, int size, Rgb c) override {
    calls.push_back({Kind::Text, x, y, textWidth(s, size), size, s, c, c});
  }
  void gradientRect(int x, int y, int w, int h, Rgb top, Rgb bottom) override {
    calls.push_back({Kind::Gradient, x, y, w, h, "", top, bottom});
  }
  void bevelRect(int x, int y, int w, int h, Rgb face, Rgb light, Rgb dark) override {
    (void)light;
    calls.push_back({Kind::Bevel, x, y, w, h, "", face, dark});
  }
  int textWidth(const std::string& s, int) const override {
    return charWidth * static_cast<int>(s.size());
  }

  bool hasText(const std::string& s) const {
    return std::any_of(calls.begin(), calls.end(), [&](const Call& c) {
      return c.kind == Kind::Text && c.text.find(s) != std::string::npos;
    });
  }
  std::vector<Call> rects(Rgb color) const {
    std::vector<Call> out;
    for (const Call& c : calls)
      if (c.kind == Kind::Fill && c.color == color) out.push_back(c);
    return out;
  }
  std::vector<Call> texts() const {
    std::vector<Call> out;
    for (const Call& c : calls)
      if (c.kind == Kind::Text) out.push_back(c);
    return out;
  }
  // Semua gambar berada di dalam jendela w x h.
  bool allInside(int w, int h) const {
    return std::all_of(calls.begin(), calls.end(), [&](const Call& c) {
      return c.x >= 0 && c.y >= 0 && c.x + c.w <= w && c.y + c.h <= h;
    });
  }

  std::vector<Call> calls;
  // Lebar per karakter; raylib asli sekitar 12-16 px pada ukuran 20.
  int charWidth = 8;
};

}  // namespace rogue
