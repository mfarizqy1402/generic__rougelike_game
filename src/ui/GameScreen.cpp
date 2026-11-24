#include "ui/GameScreen.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>

#include "core/Exceptions.hpp"
#include "core/Player.hpp"
#include "core/Types.hpp"
#include "ui/Viewport.hpp"

namespace rogue {

namespace {

// Viewport 17x15 petak; satu petak = 3 kolom karakter supaya simbol bisa
// berada tepat di tengah petak (lebar 17*3 + 2 border = 53 kolom).
constexpr int kViewW = 17;
constexpr int kViewH = 15;
constexpr int kHudWidth = 27;
constexpr std::size_t kLogLines = 4;

ftxui::Color toFtxuiColor(ColorId id) {
  using ftxui::Color;
  switch (id) {
    case ColorId::Dim:
      return Color::GrayLight;  // dinding: blok abu-abu terang agar mudah dilihat
    case ColorId::Yellow:
      return Color::Yellow;
    case ColorId::Cyan:
      return Color::Cyan;
    case ColorId::Red:
      return Color::Red;
    case ColorId::LightGreen:
      return Color::GreenLight;
    case ColorId::DarkGreen:
      return Color::DarkGreen;
    case ColorId::Brown:
      return Color::DarkOrange3;
    case ColorId::Default:
      break;
  }
  return Color::Default;
}

// Satu petak = 3 kolom. Dinding '#' digambar blok penuh; simbol lain di tengah;
// lantai kosong.
std::string cellText(char glyph) {
  if (glyph == '#') return "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88";  // U+2588 x3
  return std::string(" ") + glyph + " ";
}

// Teks petak dengan warnanya; zona aman (spasi + DarkGreen) memakai latar hijau.
ftxui::Element cellElement(const std::string& content, ColorId id, bool background) {
  using namespace ftxui;
  Element e = text(content);
  if (background) return e | bgcolor(toFtxuiColor(id));
  return e | color(toFtxuiColor(id));
}

}  // namespace

GameScreen::GameScreen(AppContext& context) : context_(context) {
  using namespace ftxui;
  Component view = Renderer([this] { return render(); });
  root_ = CatchEvent(view, [this](Event event) { return handleEvent(event); });
}

ftxui::Component GameScreen::component() { return root_; }

// ---------------------------------------------------------------------------
// Input: satu tombol = satu panggilan controller
// ---------------------------------------------------------------------------

bool GameScreen::handleEvent(const ftxui::Event& event) {
  using ftxui::Event;

  enum class Action { None, Move, Item };
  Action action = Action::None;
  Dir dir = Dir::None;
  std::size_t itemIndex = 0;

  if (event == Event::ArrowUp || event == Event::Character('w') ||
      event == Event::Character('W')) {
    action = Action::Move;
    dir = Dir::Up;
  } else if (event == Event::ArrowDown || event == Event::Character('s') ||
             event == Event::Character('S')) {
    action = Action::Move;
    dir = Dir::Down;
  } else if (event == Event::ArrowLeft || event == Event::Character('a') ||
             event == Event::Character('A')) {
    action = Action::Move;
    dir = Dir::Left;
  } else if (event == Event::ArrowRight || event == Event::Character('d') ||
             event == Event::Character('D')) {
    action = Action::Move;
    dir = Dir::Right;
  } else if (event == Event::Character(' ')) {
    action = Action::Move;  // spasi = diam
    dir = Dir::None;
  } else if (event.is_character() && event.character().size() == 1 &&
             event.character()[0] >= '1' && event.character()[0] <= '9') {
    action = Action::Item;
    itemIndex = static_cast<std::size_t>(event.character()[0] - '1');
  }

  if (action == Action::None) return false;

  try {
    if (action == Action::Move) {
      context_.controller.movePlayer(dir);
    } else {
      context_.controller.useItem(itemIndex);
    }
    message_.clear();
  } catch (const GameException& error) {
    message_ = error.what();
  }

  if (context_.controller.isOver()) context_.screens.go(ScreenId::GameOver);
  return true;
}

// ---------------------------------------------------------------------------
// Tampilan
// ---------------------------------------------------------------------------

ftxui::Element GameScreen::render() const {
  using namespace ftxui;
  try {
    Element top = hbox({
        renderMap(),
        renderHud() | size(WIDTH, EQUAL, kHudWidth),
    });

    Element footer =
        message_.empty()
            ? text("WASD/Panah: gerak | Spasi: diam | 1-9: pakai item | Esc: keluar") |
                  dim
            : text("! " + message_) | color(Color::Red);

    return vbox({top, renderLog(), renderLegend(), footer});
  } catch (const GameException& error) {
    return text(std::string("Galat: ") + error.what()) | color(Color::Red);
  }
}

ftxui::Element GameScreen::renderMap() const {
  using namespace ftxui;
  const auto grid = buildViewport(context_.controller, kViewW, kViewH);

  Elements rows;
  for (const auto& row : grid) {
    // Petak bersebelahan berwarna sama digabung jadi satu teks.
    Elements runs;
    std::string run;
    ColorId runColor = ColorId::Default;
    bool runBackground = false;
    auto flush = [&] {
      if (!run.empty()) runs.push_back(cellElement(run, runColor, runBackground));
      run.clear();
    };
    for (const Cell& cell : row) {
      const bool background = cell.color == ColorId::DarkGreen && cell.glyph == ' ';
      if (!run.empty() && (cell.color != runColor || background != runBackground)) flush();
      runColor = cell.color;
      runBackground = background;
      run += cellText(cell.glyph);
    }
    flush();
    rows.push_back(hbox(std::move(runs)));
  }
  return vbox(std::move(rows)) | border;
}

ftxui::Element GameScreen::renderHud() const {
  using namespace ftxui;
  const GameController& c = context_.controller;

  const int hp = c.playerHp();
  const float ratio = static_cast<float>(hp) / static_cast<float>(Player::kMaxHp);
  Color hpColor = Color::Green;
  if (ratio <= 0.25f) {
    hpColor = Color::Red;
  } else if (ratio <= 0.5f) {
    hpColor = Color::Yellow;
  }

  // Anggaran 15 baris isi: 4 (nama, lantai+skor, HP, gauge) + 1 (status)
  // + 1 (judul item) + 9 (item maksimum) = 15. Tidak boleh ada baris lain.
  Elements lines;
  lines.push_back(text(c.playerName()) | bold);
  lines.push_back(text("Lantai " + std::to_string(c.depth()) + "   Skor " +
                       std::to_string(c.summary().score)));
  lines.push_back(text("HP " + std::to_string(hp) + "/" +
                       std::to_string(Player::kMaxHp)) |
                  color(hpColor));
  lines.push_back(gauge(ratio) | color(hpColor));

  const std::vector<std::string> statuses = c.statusNames();
  std::string statusText = "Status: ";
  if (statuses.empty()) statusText += "bersih";
  for (std::size_t i = 0; i < statuses.size(); ++i) {
    if (i > 0) statusText += ", ";
    statusText += statuses[i];
  }
  lines.push_back(text(statusText));

  lines.push_back(text("Item:") | bold);
  const std::vector<std::string> items = c.inventoryNames();
  if (items.empty()) lines.push_back(text("  (kosong)") | dim);
  for (std::size_t i = 0; i < items.size(); ++i) {
    lines.push_back(text(" [" + std::to_string(i + 1) + "] " + items[i]));
  }

  // Tinggi HUD disamakan dengan kotak peta (15 baris isi + 2 border).
  return vbox(std::move(lines)) | border | size(HEIGHT, EQUAL, kViewH + 2);
}

ftxui::Element GameScreen::renderLegend() const {
  using namespace ftxui;
  const std::vector<LegendItem> items = legend();
  const std::size_t perRow = (items.size() + 1) / 2;

  Elements rows;
  for (std::size_t start = 0; start < items.size(); start += perRow) {
    Elements cells;
    for (std::size_t i = start; i < std::min(items.size(), start + perRow); ++i) {
      const bool background = items[i].color == ColorId::DarkGreen && items[i].glyph == ' ';
      cells.push_back(cellElement(cellText(items[i].glyph), items[i].color, background));
      cells.push_back(text(" " + items[i].label + "   "));
    }
    rows.push_back(hbox(std::move(cells)));
  }
  return vbox(std::move(rows));
}

ftxui::Element GameScreen::renderLog() const {
  using namespace ftxui;
  const auto& log = context_.controller.log();
  const std::size_t shown = std::min(kLogLines, log.size());

  Elements lines;
  for (std::size_t i = log.size() - shown; i < log.size(); ++i) {
    lines.push_back(text("> " + log[i]));
  }
  while (lines.size() < kLogLines) lines.push_back(text(""));  // tinggi tetap
  return vbox(std::move(lines));
}

}  // namespace rogue
