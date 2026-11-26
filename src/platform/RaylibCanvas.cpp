#include "platform/RaylibCanvas.hpp"

#include <raylib.h>

namespace rogue {

namespace {
Color toColor(Rgb c) { return Color{c.r, c.g, c.b, 255}; }
}  // namespace

void RaylibCanvas::fillRect(int x, int y, int w, int h, Rgb color) {
  DrawRectangle(x, y, w, h, toColor(color));
}

void RaylibCanvas::strokeRect(int x, int y, int w, int h, Rgb color) {
  DrawRectangleLines(x, y, w, h, toColor(color));
}

void RaylibCanvas::gradientRect(int x, int y, int w, int h, Rgb top, Rgb bottom) {
  DrawRectangleGradientV(x, y, w, h, toColor(top), toColor(bottom));
}

void RaylibCanvas::bevelRect(int x, int y, int w, int h, Rgb face, Rgb light, Rgb dark) {
  DrawRectangle(x, y, w, h, toColor(face));
  DrawRectangle(x, y, w, 1, toColor(light));
  DrawRectangle(x, y, 1, h, toColor(light));
  DrawRectangle(x, y + h - 1, w, 1, toColor(dark));
  DrawRectangle(x + w - 1, y, 1, h, toColor(dark));
}

void RaylibCanvas::text(const std::string& s, int x, int y, int size, Rgb color) {
  DrawText(s.c_str(), x, y, size, toColor(color));
}

int RaylibCanvas::textWidth(const std::string& s, int size) const {
  return MeasureText(s.c_str(), size);
}

}  // namespace rogue
