// Tes dasar UI grafis: Palette, InputFrame, dan RecordingCanvas.
#include <string>

#include "RecordingCanvas.hpp"
#include "check.hpp"
#include "ui/InputFrame.hpp"
#include "ui/Palette.hpp"

using namespace rogue;

namespace {

void testPalette() {
  CHECK(colorOf(ColorId::Default) == (Rgb{36, 66, 74}));   // tinta teal-slate
  CHECK(colorOf(ColorId::Dim) == (Rgb{110, 138, 146}));    // dinding + teks redup
  CHECK(colorOf(ColorId::Yellow) == (Rgb{156, 122, 28}));
  CHECK(colorOf(ColorId::Cyan) == (Rgb{58, 124, 165}));    // pemain: biru laut
  CHECK(colorOf(ColorId::Red) == (Rgb{181, 71, 58}));      // musuh: terakota
  CHECK(colorOf(ColorId::Brown) == (Rgb{154, 107, 63}));
  CHECK(colorOf(ColorId::LightGreen) == (Rgb{78, 143, 74}));
  CHECK(colorOf(ColorId::DarkGreen) == (Rgb{168, 203, 160}));  // zona aman: lumut muda
  CHECK(!(colorOf(ColorId::Red) == colorOf(ColorId::Cyan)));
  CHECK(!(colorOf(ColorId::Red) == colorOf(ColorId::DarkGreen)));
}

void testInputFrame() {
  InputFrame f;
  CHECK(!f.pressedKey(Key::Enter));
  f.pressed.push_back(Key::Enter);
  CHECK(f.pressedKey(Key::Enter));
  CHECK(!f.pressedKey(Key::Escape));
  CHECK(!f.clicked);
}

void testRecordingCanvas() {
  RecordingCanvas canvas;
  const Rgb red{230, 60, 60};
  canvas.fillRect(10, 20, 32, 32, red);
  canvas.text("Halo", 100, 50, 20, red);
  CHECK_EQ(canvas.calls.size(), static_cast<std::size_t>(2));
  CHECK(canvas.hasText("Hal"));
  CHECK(!canvas.hasText("Dunia"));
  CHECK_EQ(canvas.rects(red).size(), static_cast<std::size_t>(1));
  CHECK_EQ(canvas.rects(Rgb{0, 0, 0}).size(), static_cast<std::size_t>(0));
  CHECK(canvas.allInside(960, 680));

  RecordingCanvas wide;
  wide.text(std::string(30, 'x'), 800, 10, 20, red);  // 240 px dari x=800 -> 1040 > 960
  CHECK(!wide.allInside(960, 680));

  RecordingCanvas fancy;
  const Rgb a{1, 2, 3}, b{4, 5, 6}, c{7, 8, 9};
  fancy.gradientRect(0, 0, 50, 20, a, b);
  fancy.bevelRect(5, 5, 40, 30, a, c, b);
  CHECK_EQ(fancy.calls.size(), static_cast<std::size_t>(2));
  CHECK(fancy.calls[0].kind == RecordingCanvas::Kind::Gradient);
  CHECK(fancy.calls[0].color == a && fancy.calls[0].color2 == b);
  CHECK(fancy.calls[1].kind == RecordingCanvas::Kind::Bevel);
  CHECK(fancy.allInside(50, 35));
  CHECK(!fancy.allInside(44, 35));
}

}  // namespace

int main() {
  testPalette();
  testInputFrame();
  testRecordingCanvas();
  return check_summary();
}
