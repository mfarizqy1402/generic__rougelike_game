// Menguji GameScreen lewat canvas pencatat (tanpa jendela dan tanpa display).
#include <memory>
#include <string>
#include <vector>

#include "RecordingCanvas.hpp"
#include "check.hpp"
#include "controller/GameController.hpp"
#include "core/Achievement.hpp"
#include "core/FileScoreStore.hpp"
#include "core/Goblin.hpp"
#include "core/Hiccups.hpp"
#include "core/Player.hpp"
#include "core/Potion.hpp"
#include "core/ScoreBoard.hpp"
#include "core/StatsTracker.hpp"
#include "ui/AppContext.hpp"
#include "ui/GameScreen.hpp"
#include "ui/Palette.hpp"
#include "ui/Theme.hpp"
#include "ui/ScreenManager.hpp"

using namespace rogue;

namespace {

constexpr int kW = 960;
constexpr int kH = 680;

// Semua yang dibutuhkan sebuah GameScreen, dimiliki satu tempat.
struct Fixture {
  GameController controller;
  FileScoreStore store{"test_gamescreen_scores.txt"};
  ScoreBoard board{store};
  StatsTracker stats;
  std::vector<std::unique_ptr<Achievement>> achievements;
  ScreenManager screens;
  AppContext context{controller, board, screens, stats, achievements,
                     "", Difficulty::Normal, {}, ""};
  int gameOverScreens = 0;

  Fixture() {
    controller.startRun("Tester", "7", Difficulty::Mudah);
    class Dummy : public IScreen {
      void update(const InputFrame&) override {}
      void draw(ICanvas&) const override {}
    };
    screens.setFactory([this](ScreenId id) -> std::unique_ptr<IScreen> {
      if (id == ScreenId::GameOver) ++gameOverScreens;
      return std::make_unique<Dummy>();
    });
  }
  Player& me() {
    return static_cast<Player&>(*const_cast<Dungeon&>(controller.dungeon()).player());
  }
};

InputFrame typed(const std::string& chars) {
  InputFrame f;
  f.typed = chars;
  return f;
}

RecordingCanvas render(const GameScreen& screen) {
  RecordingCanvas canvas;
  screen.draw(canvas);
  return canvas;
}

// (a) Tembok = persegi penuh 32x32 berwarna Dim, tanpa teks '#'.
void testWallsAreSolidBlocks() {
  Fixture fx;
  GameScreen screen(fx.context);
  const RecordingCanvas canvas = render(screen);
  const auto walls = canvas.rects(colorOf(ColorId::Dim));
  int blocks = 0;
  for (const auto& w : walls)
    if (w.w == 32 && w.h == 32) ++blocks;
  CHECK(blocks > 0);
  for (const auto& t : canvas.texts()) CHECK(t.text != "#");
  CHECK(canvas.allInside(kW, kH));
}

// (b) Simbol pemain '@' cyan tepat di tengah petaknya.
void testPlayerGlyphIsCenteredInTile() {
  Fixture fx;
  GameScreen screen(fx.context);
  const RecordingCanvas canvas = render(screen);
  int found = 0;
  for (const auto& t : canvas.texts()) {
    if (t.text != "@" || !(t.color == colorOf(ColorId::Cyan))) continue;
    if (t.y > 500) continue;  // abaikan legenda
    ++found;
    const int cx = t.x + t.w / 2;
    const int cy = t.y + t.h / 2;
    CHECK_EQ((cx - 16) % 32, 16);
    CHECK_EQ((cy - 16) % 32, 16);
  }
  CHECK_EQ(found, 1);
}

// (c) Tas penuh (9 item) dan satu status: HUD menampilkan [1]..[9], tetap muat.
void testHudShowsFullInventory() {
  Fixture fx;
  while (fx.me().inventory().size() < 9) fx.me().inventory().push_back(std::make_shared<Potion>());
  fx.me().addStatus(std::make_unique<Hiccups>(5));
  GameScreen screen(fx.context);
  const RecordingCanvas canvas = render(screen);
  for (int i = 1; i <= 9; ++i) CHECK(canvas.hasText("[" + std::to_string(i) + "]"));
  CHECK(canvas.hasText("Status:"));
  CHECK(canvas.allInside(kW, kH));
}

// (d) Legenda memuat entri tetap; simbol musuh berwarna merah.
void testLegend() {
  Fixture fx;
  GameScreen screen(fx.context);
  const RecordingCanvas canvas = render(screen);
  CHECK(canvas.hasText("Zona aman"));
  CHECK(canvas.hasText("Goblin"));
  CHECK(canvas.hasText("Tembok"));
  int redGoblinSymbols = 0;
  for (const auto& t : canvas.texts())
    if (t.text == "g" && t.y >= 590 && t.color == colorOf(ColorId::Red)) ++redGoblinSymbols;
  CHECK_EQ(redGoblinSymbols, 1);
}

// Tema: latar langit penuh, lantai peta berupa panel kaca, HUD berupa jendela.
void testThemedLook() {
  Fixture fx;
  GameScreen screen(fx.context);
  const RecordingCanvas canvas = render(screen);
  CHECK(canvas.calls.front().kind == RecordingCanvas::Kind::Gradient);
  CHECK(canvas.calls.front().w == kW && canvas.calls.front().h == kH);
  bool floor = false;
  for (const auto& r : canvas.rects(theme::kGlass))
    if (r.x == 16 && r.y == 16 && r.w == 544 && r.h == 480) floor = true;
  CHECK(floor);
  CHECK(canvas.hasText("Status"));
  CHECK(canvas.hasText("[_][o][x]"));
  CHECK(canvas.allInside(kW, kH));
  RecordingCanvas wide;
  wide.charWidth = 16;
  screen.draw(wide);
  CHECK(wide.allInside(kW, kH));
}

// (e) + (f) Satu tombol = satu panggilan controller; aksi gagal jadi pesan.
void testInputMapsToController() {
  Fixture fx;
  GameScreen screen(fx.context);

  // Tas kosong lebih kecil dari 9: slot 9 tidak ada -> pesan galat, bukan crash.
  while (fx.me().inventory().size() >= 9) fx.me().inventory().pop_back();
  screen.update(typed("9"));
  CHECK(render(screen).hasText("! Item tidak ditemukan."));

  // Gerak memanggil controller dan menghapus pesan lama.
  const Pos before = fx.controller.playerPos();
  int moved = 0;
  for (const char* key : {"d", "a", "w", "s", "D", "A", "W", "S"}) {
    screen.update(typed(key));
    if (!(fx.controller.playerPos() == before)) ++moved;
  }
  CHECK(moved > 0);
  CHECK(!render(screen).hasText("! Item tidak ditemukan."));

  // Panah dan spasi juga diterima.
  InputFrame arrow;
  arrow.pressed.push_back(Key::Left);
  screen.update(arrow);
  screen.update(typed(" "));
  CHECK(!fx.controller.isOver());

  // Pakai item: Potion di slot 1.
  fx.me().inventory().clear();
  fx.me().inventory().push_back(std::make_shared<Potion>());
  screen.update(typed("1"));
  CHECK(fx.me().inventory().empty());
}

// (g) Run selesai: pindah ke Game Over tepat sekali, input sesudahnya aman.
void testGameOverIsScheduledOnce() {
  Fixture fx;
  GameScreen screen(fx.context);
  // Sisa 1 HP, Goblin di sebelah pemain: giliran berikutnya membunuh pemain.
  fx.me().takeDamage(Player::kMaxHp - 1);
  Dungeon& d = const_cast<Dungeon&>(fx.controller.dungeon());
  const Pos here = fx.controller.playerPos();
  bool placed = false;
  for (const Pos p : {Pos{here.x + 1, here.y}, Pos{here.x - 1, here.y}, Pos{here.x, here.y + 1},
                      Pos{here.x, here.y - 1}}) {
    if (d.inBounds(p) && d.tile(p).isWalkable() && d.entityAt(p) == nullptr) {
      d.add(std::make_unique<Goblin>(p));
      placed = true;
      break;
    }
  }
  CHECK(placed);
  screen.update(typed(" "));
  CHECK(fx.controller.isOver());
  CHECK(fx.screens.applyPending());
  CHECK_EQ(fx.gameOverScreens, 1);
  for (int i = 0; i < 3; ++i) screen.update(typed("d"));  // tombol ditahan
  CHECK(!fx.screens.applyPending());
  CHECK_EQ(fx.gameOverScreens, 1);
}

// (h) Kasus terburuk tata letak: log 4 baris panjang dan nama 16 karakter.
void testWorstCaseLayoutFits() {
  Fixture fx;
  fx.controller.startRun("ABCDEFGHIJKLMNOP", "7", Difficulty::Mudah);
  GameScreen screen(fx.context);
  while (fx.me().inventory().size() < 9) fx.me().inventory().push_back(std::make_shared<Potion>());
  for (int i = 0; i < 8; ++i) screen.update(typed(i % 2 == 0 ? "d" : "a"));
  const RecordingCanvas canvas = render(screen);
  CHECK(canvas.hasText("ABCDEFGHIJKLMNOP"));
  CHECK(canvas.allInside(kW, kH));
}

}  // namespace

int main() {
  testWallsAreSolidBlocks();
  testPlayerGlyphIsCenteredInTile();
  testHudShowsFullInventory();
  testLegend();
  testThemedLook();
  testInputMapsToController();
  testGameOverIsScheduledOnce();
  testWorstCaseLayoutFits();
  return check_summary();
}
