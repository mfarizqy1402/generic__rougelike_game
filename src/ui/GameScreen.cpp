#include "ui/GameScreen.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "core/Exceptions.hpp"
#include "core/Player.hpp"
#include "core/Types.hpp"
#include "ui/Palette.hpp"
#include "ui/Theme.hpp"
#include "ui/Viewport.hpp"

namespace rogue {

namespace {

// Viewport 17x15 petak; satu petak = 32 px.
constexpr int kViewW = 17;
constexpr int kViewH = 15;
constexpr int kTile = 32;
constexpr std::size_t kLogLines = 4;
constexpr std::size_t kMaxHudItems = 9;  // tombol 1-9

constexpr int kMapX = 16;
constexpr int kMapY = 16;
constexpr int kHudX = 584;
constexpr int kHudW = 360;
constexpr int kHudLine = 26;
constexpr int kLogY = 508;
constexpr int kLogLine = 20;
constexpr int kLegendY = 596;
constexpr int kLegendRow = 24;
constexpr int kFooterY = 656;
constexpr int kWindowW = 960;
constexpr int kGlyphSize = 30;
constexpr int kTextSize = 18;
constexpr int kSmallSize = 16;

// Tinta utama tema (teks gelap di atas panel kaca terang).
Rgb ink() { return colorOf(ColorId::Default); }

constexpr int kMapW = kViewW * kTile;
constexpr int kMapH = kViewH * kTile;
constexpr int kHudH = kMapH + 2;
constexpr int kLogPanelY = 500;
constexpr int kLogPanelH = 88;
constexpr int kLegendPanelY = 592;
constexpr int kLegendPanelH = 54;

// Satu petak legenda/peta pada (x, y) berukuran 32 px.
void drawCell(ICanvas& canvas, int x, int y, char glyph, ColorId color) {
  if (glyph == '#') {
    canvas.fillRect(x, y, kTile, kTile, colorOf(color));
  } else if (glyph == ' ') {
    if (color == ColorId::DarkGreen) canvas.fillRect(x, y, kTile, kTile, colorOf(color));
  } else {
    const std::string s(1, glyph);
    const int w = canvas.textWidth(s, kGlyphSize);
    canvas.text(s, x + (kTile - w) / 2, y + (kTile - kGlyphSize) / 2, kGlyphSize, colorOf(color));
  }
}

}  // namespace

GameScreen::GameScreen(AppContext& context) : context_(context) {}

// ---------------------------------------------------------------------------
// Input: satu tombol = satu panggilan controller
// ---------------------------------------------------------------------------

void GameScreen::update(const InputFrame& input) {
  if (finished_) return;

  for (Key key : input.pressed) {
    switch (key) {
      case Key::Up:    perform(Action::Move, static_cast<int>(Dir::Up)); break;
      case Key::Down:  perform(Action::Move, static_cast<int>(Dir::Down)); break;
      case Key::Left:  perform(Action::Move, static_cast<int>(Dir::Left)); break;
      case Key::Right: perform(Action::Move, static_cast<int>(Dir::Right)); break;
      default: break;
    }
    if (finished_) return;
  }

  for (char ch : input.typed) {
    switch (ch) {
      case 'w': case 'W': perform(Action::Move, static_cast<int>(Dir::Up)); break;
      case 's': case 'S': perform(Action::Move, static_cast<int>(Dir::Down)); break;
      case 'a': case 'A': perform(Action::Move, static_cast<int>(Dir::Left)); break;
      case 'd': case 'D': perform(Action::Move, static_cast<int>(Dir::Right)); break;
      case ' ': perform(Action::Move, static_cast<int>(Dir::None)); break;  // spasi = diam
      default:
        if (ch >= '1' && ch <= '9') perform(Action::Item, ch - '1');
        break;
    }
    if (finished_) return;
  }
}

void GameScreen::perform(Action action, int dirOrIndex) {
  try {
    if (action == Action::Move) {
      context_.controller.movePlayer(static_cast<Dir>(dirOrIndex));
    } else {
      context_.controller.useItem(static_cast<std::size_t>(dirOrIndex));
    }
    message_.clear();
  } catch (const GameException& error) {
    message_ = error.what();
  }

  if (context_.controller.isOver()) {
    finished_ = true;
    context_.screens.go(ScreenId::GameOver);
  }
}

// ---------------------------------------------------------------------------
// Tampilan
// ---------------------------------------------------------------------------

void GameScreen::draw(ICanvas& canvas) const {
  try {
    theme::background(canvas, kWindowW, 680);
    drawMap(canvas);
    drawHud(canvas);
    drawLog(canvas);
    drawLegend(canvas);

    if (message_.empty()) {
      canvas.text(theme::fit(canvas, "WASD/Panah: gerak | Spasi: diam | 1-9: pakai item | Esc: keluar",
                             kSmallSize, kWindowW - 2 * kMapX),
                  kMapX, kFooterY, kSmallSize, ink());
    } else {
      canvas.text(theme::fit(canvas, "! " + message_, kSmallSize, kWindowW - 2 * kMapX), kMapX, kFooterY,
                  kSmallSize, colorOf(ColorId::Red));
    }
  } catch (const GameException& error) {
    canvas.text(theme::fit(canvas, std::string("Galat: ") + error.what(), kTextSize, kWindowW - 2 * kMapX),
                kMapX, kMapY, kTextSize, colorOf(ColorId::Red));
  }
}

void GameScreen::drawMap(ICanvas& canvas) const {
  const auto grid = buildViewport(context_.controller, kViewW, kViewH);
  theme::panel(canvas, kMapX, kMapY, kMapW, kMapH);  // lantai: kaca terang
  for (int row = 0; row < kViewH; ++row) {
    for (int col = 0; col < kViewW; ++col) {
      const Cell& cell = grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
      drawCell(canvas, kMapX + col * kTile, kMapY + row * kTile, cell.glyph, cell.color);
    }
  }
  canvas.strokeRect(kMapX - 1, kMapY - 1, kMapW + 2, kMapH + 2, theme::kSilverDark);
}

void GameScreen::drawHud(ICanvas& canvas) const {
  const GameController& c = context_.controller;
  const int innerX = kHudX + 12;
  const int innerW = kHudW - 24;
  int y = kMapY - 1 + theme::kTitleBarH + 8;

  const int hp = c.playerHp();
  const float ratio = static_cast<float>(hp) / static_cast<float>(Player::kMaxHp);
  Rgb hpColor = colorOf(ColorId::LightGreen);
  if (ratio <= 0.25f) {
    hpColor = colorOf(ColorId::Red);
  } else if (ratio <= 0.5f) {
    hpColor = colorOf(ColorId::Yellow);
  }

  theme::window(canvas, kHudX, kMapY - 1, kHudW, kHudH, "Status");

  // Anggaran 15 baris isi: 4 (nama, lantai+skor, HP, gauge) + 1 (status)
  // + 1 (judul item) + 9 (item maksimum) = 15.
  canvas.text(theme::fit(canvas, c.playerName(), kTextSize, innerW), innerX, y, kTextSize, ink());
  y += kHudLine;
  canvas.text(theme::fit(canvas,
                  "Lantai " + std::to_string(c.depth()) + "   Skor " +
                      std::to_string(c.summary().score),
                  kTextSize, innerW),
              innerX, y, kTextSize, ink());
  y += kHudLine;
  canvas.text("HP " + std::to_string(hp) + "/" + std::to_string(Player::kMaxHp), innerX, y,
              kTextSize, hpColor);
  y += kHudLine;
  const int gaugeW = std::max(0, std::min(innerW, static_cast<int>(ratio * innerW)));
  canvas.fillRect(innerX, y, innerW, 14, theme::kSilver);  // jalur slider
  if (gaugeW > 0) canvas.fillRect(innerX, y, gaugeW, 14, hpColor);
  canvas.strokeRect(innerX, y, innerW, 14, theme::kSilverDark);
  y += kHudLine;

  const std::vector<std::string> statuses = c.statusNames();
  std::string statusText = "Status: ";
  if (statuses.empty()) statusText += "bersih";
  for (std::size_t i = 0; i < statuses.size(); ++i) {
    if (i > 0) statusText += ", ";
    statusText += statuses[i];
  }
  canvas.text(theme::fit(canvas, statusText, kTextSize, innerW), innerX, y, kTextSize, ink());
  y += kHudLine;

  canvas.text("Item:", innerX, y, kTextSize, colorOf(ColorId::Yellow));
  y += kHudLine;
  const std::vector<std::string> items = c.inventoryNames();
  if (items.empty()) {
    canvas.text("  (kosong)", innerX, y, kTextSize, colorOf(ColorId::Dim));
  }
  for (std::size_t i = 0; i < items.size() && i < kMaxHudItems; ++i) {
    canvas.text(theme::fit(canvas, " [" + std::to_string(i + 1) + "] " + items[i], kTextSize, innerW),
                innerX, y, kTextSize, ink());
    y += kHudLine;
  }
}

void GameScreen::drawLog(ICanvas& canvas) const {
  theme::panel(canvas, kMapX - 4, kLogPanelY, kWindowW - 2 * (kMapX - 4), kLogPanelH);
  const auto& log = context_.controller.log();
  const std::size_t shown = std::min(kLogLines, log.size());
  int y = kLogY;
  for (std::size_t i = log.size() - shown; i < log.size(); ++i) {
    canvas.text(theme::fit(canvas, "> " + log[i], kSmallSize, kWindowW - 2 * kMapX), kMapX, y,
                kSmallSize, ink());
    y += kLogLine;
  }
}

void GameScreen::drawLegend(ICanvas& canvas) const {
  const std::vector<LegendItem> items = legend();
  const std::size_t perRow = (items.size() + 1) / 2;
  const int colW = (kWindowW - 2 * kMapX) / static_cast<int>(perRow);
  theme::panel(canvas, kMapX - 4, kLegendPanelY, kWindowW - 2 * (kMapX - 4), kLegendPanelH);

  for (std::size_t i = 0; i < items.size(); ++i) {
    const int col = static_cast<int>(i % perRow);
    const int row = static_cast<int>(i / perRow);
    const int x = kMapX + col * colW;
    const int y = kLegendY + row * kLegendRow;

    // Simbol kecil (setengah petak) lalu nama.
    const LegendItem& item = items[i];
    if (item.glyph == '#' || (item.glyph == ' ' && item.color == ColorId::DarkGreen)) {
      canvas.fillRect(x, y, 20, 20, colorOf(item.color));
    } else {
      const std::string s(1, item.glyph);
      canvas.text(s, x + (20 - canvas.textWidth(s, kSmallSize)) / 2, y + 2, kSmallSize,
                  colorOf(item.color));
    }
    canvas.text(theme::fit(canvas, item.label, kSmallSize, colW - 30), x + 28, y + 2, kSmallSize, ink());
  }
}

}  // namespace rogue
