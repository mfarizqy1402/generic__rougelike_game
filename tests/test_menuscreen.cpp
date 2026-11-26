// Menguji MenuScreen lewat canvas pencatat (tanpa jendela dan tanpa display).
#include <memory>
#include <string>
#include <vector>

#include "RecordingCanvas.hpp"
#include "check.hpp"
#include "controller/GameController.hpp"
#include "core/Achievement.hpp"
#include "core/FileScoreStore.hpp"
#include "core/ScoreBoard.hpp"
#include "core/StatsTracker.hpp"
#include "ui/AppContext.hpp"
#include "ui/MenuScreen.hpp"
#include "ui/ScreenManager.hpp"
#include "ui/Theme.hpp"

using namespace rogue;

namespace {

constexpr int kW = 960;
constexpr int kH = 680;

struct Fixture {
  GameController controller;
  FileScoreStore store{"test_menuscreen_scores.txt"};
  ScoreBoard board{store};
  StatsTracker stats;
  std::vector<std::unique_ptr<Achievement>> achievements;
  ScreenManager screens;
  int quits = 0;
  std::vector<ScreenId> requested;
  AppContext context{controller, board, screens, stats, achievements,
                     "", Difficulty::Normal, {}, ""};

  Fixture() {
    context.quit = [this] { ++quits; };
    class Dummy : public IScreen {
      void update(const InputFrame&) override {}
      void draw(ICanvas&) const override {}
    };
    screens.setFactory([this](ScreenId id) -> std::unique_ptr<IScreen> {
      requested.push_back(id);
      return std::make_unique<Dummy>();
    });
  }
  bool wentToGame() {
    screens.applyPending();
    return !requested.empty() && requested.back() == ScreenId::Game;
  }
};

InputFrame typed(const std::string& chars) {
  InputFrame f;
  f.typed = chars;
  return f;
}
InputFrame key(Key k) {
  InputFrame f;
  f.pressed.push_back(k);
  return f;
}
RecordingCanvas render(const MenuScreen& menu) {
  RecordingCanvas canvas;
  menu.draw(canvas);
  return canvas;
}
void tab(MenuScreen& menu, int times) {
  for (int i = 0; i < times; ++i) menu.update(key(Key::Tab));
}
// Pusat label teks pertama yang memuat `label`.
bool centerOf(const RecordingCanvas& canvas, const std::string& label, int& cx, int& cy) {
  for (const auto& t : canvas.texts()) {
    if (t.text == label) {
      cx = t.x + t.w / 2;
      cy = t.y + t.h / 2;
      return true;
    }
  }
  return false;
}

void testTitleAndTagline() {
  Fixture fx;
  MenuScreen menu(fx.context);
  const RecordingCanvas canvas = render(menu);
  CHECK(canvas.hasText("G E N E R I C   R O G U E L I K E   G A M E"));
  CHECK(!canvas.hasText("D U N G E O N"));
  CHECK(canvas.hasText("Tembok tidak minta maaf"));
  CHECK(canvas.allInside(kW, kH));
}

void testTypeNameAndStart() {
  Fixture fx;
  MenuScreen menu(fx.context);
  menu.update(typed("Teach"));
  CHECK(render(menu).hasText("Mulai"));
  tab(menu, 3);  // Nama -> Seed -> Kesulitan -> Mulai
  menu.update(key(Key::Enter));
  CHECK(fx.wentToGame());
  CHECK_EQ(fx.controller.playerName(), std::string("Teach"));
  CHECK_EQ(fx.context.lastName, std::string("Teach"));
}

void testInvalidNameHidesStart() {
  Fixture fx;
  MenuScreen menu(fx.context);
  menu.update(typed("ab"));
  const RecordingCanvas canvas = render(menu);
  CHECK(!canvas.hasText("Mulai"));
  CHECK(canvas.hasText("Nama minimal 3 karakter."));
  CHECK(canvas.allInside(kW, kH));
  // Enter di kolom Nama tidak memulai run (hanya pindah ke Seed); Tab
  // melewati Mulai langsung ke Keluar.
  menu.update(key(Key::Enter));
  CHECK(!fx.wentToGame());
  tab(menu, 2);  // Kesulitan, Keluar (Mulai dilewati)
  menu.update(key(Key::Enter));
  CHECK_EQ(fx.quits, 1);
}

// Review Focus 3: ketikan aneh tidak merusak atau masuk kolom.
void testStrangeTypingIsFiltered() {
  Fixture fx;
  MenuScreen menu(fx.context);
  menu.update(typed("\xC3\xA9"));       // non-ASCII (UTF-8 'e' beraksen)
  menu.update(typed(std::string(1, '\x01')));  // karakter kontrol
  menu.update(typed("!@#"));
  for (int i = 0; i < 5; ++i) menu.update(key(Key::Backspace));  // kosong: aman
  CHECK(!render(menu).hasText("Mulai"));  // nama masih kosong

  menu.update(typed(std::string(40, 'a')));
  menu.update(typed("A B"));  // spasi diterima, tetapi sudah penuh (16)
  RecordingCanvas canvas = render(menu);
  CHECK(canvas.hasText(std::string(16, 'a')));
  CHECK(!canvas.hasText(std::string(17, 'a')));
  CHECK(canvas.allInside(kW, kH));

  tab(menu, 1);  // Seed: hanya digit, maksimum 9
  menu.update(typed("12ab34"));
  menu.update(typed(std::string(20, '7')));
  canvas = render(menu);
  CHECK(canvas.hasText("123477777"));  // "1234" + lima '7' = 9 digit
  CHECK(!canvas.hasText("1234777777"));
  CHECK(canvas.allInside(kW, kH));
}

void testSpaceInNameAndFocusWrap() {
  Fixture fx;
  MenuScreen menu(fx.context);
  menu.update(typed("A B"));
  CHECK(render(menu).hasText("A B"));
  CHECK(render(menu).hasText("Mulai"));
  menu.update(key(Key::Up));  // melingkar ke tombol terakhir (Keluar)
  menu.update(key(Key::Enter));
  CHECK_EQ(fx.quits, 1);
}

void testDifficultySelection() {
  Fixture fx;
  MenuScreen menu(fx.context);
  menu.update(typed("Teach"));
  tab(menu, 2);  // fokus Kesulitan (bawaan Normal)
  menu.update(key(Key::Right));
  menu.update(key(Key::Right));  // dijepit di Sulit
  menu.update(key(Key::Down));   // Mulai
  menu.update(key(Key::Enter));
  CHECK(fx.wentToGame());
  CHECK(fx.context.lastDifficulty == Difficulty::Sulit);
}

void testClickQuitButton() {
  Fixture fx;
  MenuScreen menu(fx.context);
  int cx = 0, cy = 0;
  CHECK(centerOf(render(menu), "Keluar", cx, cy));
  InputFrame click;
  click.clicked = true;
  click.mouseX = cx;
  click.mouseY = cy;
  menu.update(click);
  CHECK_EQ(fx.quits, 1);
}

void testClickStartButton() {
  Fixture fx;
  MenuScreen menu(fx.context);
  menu.update(typed("Teach"));
  int cx = 0, cy = 0;
  CHECK(centerOf(render(menu), "Mulai", cx, cy));
  InputFrame click;
  click.clicked = true;
  click.mouseX = cx;
  click.mouseY = cy;
  menu.update(click);
  CHECK(fx.wentToGame());
}

void testMusicStatus() {
  Fixture fx;
  fx.context.musicStatus = "Musik: 3 lagu";
  MenuScreen with(fx.context);
  CHECK(render(with).hasText("Musik: 3 lagu"));
  fx.context.musicStatus = "Musik: tidak ada (isi folder music/)";
  MenuScreen none(fx.context);
  const RecordingCanvas canvas = render(none);
  CHECK(canvas.hasText("tidak ada"));
  CHECK(canvas.allInside(kW, kH));
  fx.context.musicStatus.clear();
  MenuScreen empty(fx.context);
  CHECK(!render(empty).hasText("Musik:"));
}

int count(const RecordingCanvas& c, RecordingCanvas::Kind k) {
  int n = 0;
  for (const auto& call : c.calls)
    if (call.kind == k) ++n;
  return n;
}

// Tema Frutiger Eco/Y2K: latar langit penuh, jendela dengan title bar, tombol perak.
void testThemedLook() {
  Fixture fx;
  MenuScreen menu(fx.context);
  RecordingCanvas empty = render(menu);
  CHECK(empty.calls.front().kind == RecordingCanvas::Kind::Gradient);
  CHECK(empty.calls.front().w == kW && empty.calls.front().h == kH);
  CHECK(empty.hasText("[_][o][x]"));
  CHECK(empty.hasText("generic_roguelike_game"));
  CHECK_EQ(count(empty, RecordingCanvas::Kind::Bevel), 2);  // pilihan kesulitan + Keluar
  menu.update(typed("Teach"));
  RecordingCanvas ready = render(menu);
  CHECK_EQ(count(ready, RecordingCanvas::Kind::Bevel), 3);  // + Mulai
  CHECK(ready.allInside(kW, kH));
  RecordingCanvas wide;
  wide.charWidth = 16;
  menu.draw(wide);
  CHECK(wide.allInside(kW, kH));
}

}  // namespace

int main() {
  testTitleAndTagline();
  testTypeNameAndStart();
  testInvalidNameHidesStart();
  testStrangeTypingIsFiltered();
  testSpaceInNameAndFocusWrap();
  testDifficultySelection();
  testClickQuitButton();
  testClickStartButton();
  testMusicStatus();
  testThemedLook();
  return check_summary();
}
