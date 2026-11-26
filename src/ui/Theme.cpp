#include "ui/Theme.hpp"

namespace rogue {
namespace theme {

const Rgb kSkyTop{220, 239, 243};
const Rgb kSkyBottom{185, 221, 230};
const Rgb kAquaTop{80, 158, 168};
const Rgb kAquaBottom{58, 133, 144};
const Rgb kTitleText{244, 251, 250};
const Rgb kGlass{234, 246, 244};
const Rgb kEdge{143, 163, 170};
const Rgb kSilver{201, 214, 218};
const Rgb kSilverLight{244, 251, 250};
const Rgb kSilverDark{111, 135, 144};
const Rgb kLeaf{111, 168, 106};
const Rgb kInk{36, 66, 74};

namespace {
constexpr int kCaptionSize = 16;
constexpr int kTitleSize = 16;
}  // namespace

bool inRect(int px, int py, int x, int y, int w, int h) {
  return px >= x && px < x + w && py >= y && py < y + h;
}

std::string fit(const ICanvas& canvas, std::string s, int size, int maxWidth) {
  if (canvas.textWidth(s, size) <= maxWidth) return s;
  while (!s.empty() && canvas.textWidth(s + "...", size) > maxWidth) s.pop_back();
  return s + "...";
}

void background(ICanvas& canvas, int w, int h) {
  canvas.gradientRect(0, 0, w, h, kSkyTop, kSkyBottom);
}

void panel(ICanvas& canvas, int x, int y, int w, int h) {
  canvas.fillRect(x, y, w, h, kGlass);
  canvas.strokeRect(x, y, w, h, kEdge);
}

void window(ICanvas& canvas, int x, int y, int w, int h, const std::string& title) {
  panel(canvas, x, y, w, h);
  canvas.gradientRect(x, y, w, kTitleBarH, kAquaTop, kAquaBottom);
  canvas.strokeRect(x, y, w, kTitleBarH, kAquaBottom);

  const std::string caption = "[_][o][x]";
  const int capW = canvas.textWidth(caption, kCaptionSize);
  const int textY = y + (kTitleBarH - kTitleSize) / 2;
  const bool captionFits = capW + 24 < w;
  if (captionFits) canvas.text(caption, x + w - capW - 8, textY, kCaptionSize, kTitleText);
  const int titleMax = w - 16 - (captionFits ? capW + 8 : 0);
  canvas.text(fit(canvas, title, kTitleSize, titleMax), x + 8, textY, kTitleSize, kTitleText);
}

void button(ICanvas& canvas, int x, int y, int w, int h, const std::string& label, int size,
            bool focused) {
  canvas.bevelRect(x, y, w, h, focused ? kLeaf : kSilver, kSilverLight, kSilverDark);
  const std::string shown = fit(canvas, label, size, w - 8);
  const int tw = canvas.textWidth(shown, size);
  canvas.text(shown, x + (w - tw) / 2, y + (h - size) / 2, size, kInk);
}

}  // namespace theme
}  // namespace rogue
