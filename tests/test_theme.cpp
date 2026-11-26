// Tes tema Frutiger Eco / Y2K: palet bebas neon, kontras teks, dan widget bersama.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

#include "RecordingCanvas.hpp"
#include "check.hpp"
#include "ui/Palette.hpp"
#include "ui/Theme.hpp"

using namespace rogue;

namespace {

int chroma(Rgb c) {
  return std::max({c.r, c.g, c.b}) - std::min({c.r, c.g, c.b});
}

double luminance(Rgb c) {
  auto lin = [](int v) {
    const double s = v / 255.0;
    return s <= 0.03928 ? s / 12.92 : std::pow((s + 0.055) / 1.055, 2.4);
  };
  return 0.2126 * lin(c.r) + 0.7152 * lin(c.g) + 0.0722 * lin(c.b);
}

double contrast(Rgb a, Rgb b) {
  const double la = luminance(a), lb = luminance(b);
  return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

const ColorId kAllColors[] = {ColorId::Default, ColorId::Dim,   ColorId::Yellow,
                              ColorId::Cyan,    ColorId::Red,   ColorId::Brown,
                              ColorId::LightGreen, ColorId::DarkGreen};

// Tidak ada warna neon: selisih kanal maksimum-minimum <= 140 (0,55).
void testNoNeon() {
  for (ColorId id : kAllColors) CHECK(chroma(colorOf(id)) <= 140);
  const Rgb chrome[] = {theme::kSkyTop,  theme::kSkyBottom,   theme::kAquaTop,  theme::kAquaBottom,
                        theme::kGlass,   theme::kEdge,        theme::kSilver,   theme::kSilverLight,
                        theme::kSilverDark, theme::kLeaf,     theme::kInk};
  for (const Rgb& c : chrome) CHECK(chroma(c) <= 140);
}

// Teks harus terbaca di atas panel kaca (WCAG >= 3:1 untuk teks 16-20 px).
void testTextContrast() {
  const ColorId text[] = {ColorId::Default, ColorId::Dim, ColorId::Yellow, ColorId::Red,
                          ColorId::LightGreen};
  for (ColorId id : text) CHECK(contrast(colorOf(id), theme::kGlass) >= 3.0);
  CHECK(contrast(theme::kTitleText, theme::kAquaBottom) >= 3.0);
  // Teks judul (y 5..21 dari bar 26 px) berada di tengah gradasi, bukan di dasarnya:
  // uji warna paling terang di zona itu (t = 5/26 ~ 0,2).
  const Rgb lightestUnderText{
      static_cast<std::uint8_t>(theme::kAquaTop.r + (theme::kAquaBottom.r - theme::kAquaTop.r) * 0.2),
      static_cast<std::uint8_t>(theme::kAquaTop.g + (theme::kAquaBottom.g - theme::kAquaTop.g) * 0.2),
      static_cast<std::uint8_t>(theme::kAquaTop.b + (theme::kAquaBottom.b - theme::kAquaTop.b) * 0.2)};
  CHECK(contrast(theme::kTitleText, lightestUnderText) >= 3.0);
  CHECK(contrast(theme::kInk, theme::kSilver) >= 4.5);
  CHECK(contrast(theme::kInk, theme::kLeaf) >= 3.0);
}

// Entitas tetap berbeda satu sama lain dan dari latar peta.
void testEntitiesDistinct() {
  CHECK(!(colorOf(ColorId::Red) == colorOf(ColorId::Cyan)));
  CHECK(!(colorOf(ColorId::Yellow) == colorOf(ColorId::Brown)));
  CHECK(contrast(colorOf(ColorId::Cyan), colorOf(ColorId::DarkGreen)) >= 2.0);
  CHECK(contrast(colorOf(ColorId::Red), colorOf(ColorId::DarkGreen)) >= 2.0);
  CHECK(contrast(colorOf(ColorId::Dim), theme::kSkyBottom) >= 2.0);  // dinding vs lantai
}

void testBackground() {
  RecordingCanvas c;
  theme::background(c, 960, 680);
  CHECK_EQ(c.calls.size(), static_cast<std::size_t>(1));
  const auto& g = c.calls[0];
  CHECK(g.kind == RecordingCanvas::Kind::Gradient);
  CHECK(g.x == 0 && g.y == 0 && g.w == 960 && g.h == 680);
  CHECK(g.color == theme::kSkyTop && g.color2 == theme::kSkyBottom);
}

void testWindow() {
  RecordingCanvas c;
  theme::window(c, 100, 50, 300, 200, "Status");
  bool bar = false;
  for (const auto& k : c.calls)
    if (k.kind == RecordingCanvas::Kind::Gradient && k.x == 100 && k.y == 50 && k.w == 300 &&
        k.h == theme::kTitleBarH && k.color == theme::kAquaTop && k.color2 == theme::kAquaBottom)
      bar = true;
  CHECK(bar);
  CHECK(c.hasText("Status"));
  CHECK(c.hasText("[_][o][x]"));
  for (const auto& t : c.texts()) {
    CHECK(t.y >= 50 && t.y + t.h <= 50 + theme::kTitleBarH);  // judul di dalam bar
    CHECK(t.x >= 100 && t.x + t.w <= 400);
  }
  CHECK(c.allInside(400, 250));
  // Judul panjang dipotong, bukan meluber.
  RecordingCanvas narrow;
  theme::window(narrow, 0, 0, 120, 100, std::string(40, 'J'));
  CHECK(narrow.allInside(120, 100));
}

void testButton() {
  RecordingCanvas idle, focused;
  theme::button(idle, 10, 10, 140, 40, "Mulai", 20, false);
  theme::button(focused, 10, 10, 140, 40, "Mulai", 20, true);
  CHECK(idle.hasText("Mulai") && focused.hasText("Mulai"));
  CHECK(idle.calls[0].kind == RecordingCanvas::Kind::Bevel);
  CHECK(idle.calls[0].color == theme::kSilver);
  CHECK(focused.calls[0].color == theme::kLeaf);
  CHECK(idle.allInside(150, 50) && focused.allInside(150, 50));
  const auto t = idle.texts()[0];  // teks di tengah tombol
  CHECK_EQ(t.x + t.w / 2, 10 + 70);
}

void testHelpers() {
  RecordingCanvas c;
  CHECK_EQ(theme::fit(c, "abc", 16, 100), std::string("abc"));
  const std::string cut = theme::fit(c, std::string(40, 'x'), 16, 80);
  CHECK(c.textWidth(cut, 16) <= 80);
  CHECK(cut.size() > 3 && cut.substr(cut.size() - 3) == "...");
  CHECK(theme::inRect(5, 5, 0, 0, 10, 10));
  CHECK(!theme::inRect(10, 5, 0, 0, 10, 10));
}

}  // namespace

int main() {
  testNoNeon();
  testTextContrast();
  testEntitiesDistinct();
  testBackground();
  testWindow();
  testButton();
  testHelpers();
  return check_summary();
}
