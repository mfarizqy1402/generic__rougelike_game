// Menguji GameOverScreen lewat canvas pencatat (tanpa jendela dan tanpa display).
#include <memory>
#include <string>
#include <vector>

#include "RecordingCanvas.hpp"
#include "check.hpp"
#include "controller/GameController.hpp"
#include "core/Achievement.hpp"
#include "core/Exceptions.hpp"
#include "core/IScoreStore.hpp"
#include "core/Player.hpp"
#include "core/ScoreBoard.hpp"
#include "core/StatsTracker.hpp"
#include "ui/AppContext.hpp"
#include "ui/GameOverScreen.hpp"
#include "ui/ScreenManager.hpp"
#include "ui/Theme.hpp"

using namespace rogue;

namespace {

constexpr int kW = 960;
constexpr int kH = 680;

class MemoryStore : public IScoreStore {
 public:
  std::vector<ScoreEntry> load() override { return data; }
  void save(const std::vector<ScoreEntry>& entries) override {
    ++saves;
    data = entries;
  }
  std::vector<ScoreEntry> data;
  int saves = 0;
};

class FailingStore : public IScoreStore {
 public:
  std::vector<ScoreEntry> load() override { return {}; }
  void save(const std::vector<ScoreEntry>&) override {
    throw ScoreFileException("Gagal menyimpan skor.");
  }
};

class AlwaysUnlocked : public Achievement {
 public:
  AlwaysUnlocked() : Achievement("Selalu Terbuka") {}

 protected:
  void handle(const GameEvent&) override { unlock(); }
  void reset() override {}
};

struct Fixture {
  explicit Fixture(IScoreStore& s) : board(s) {
    controller.startRun("Tester", "7", Difficulty::Mudah);
    context.quit = [] {};
    class Dummy : public IScreen {
      void update(const InputFrame&) override {}
      void draw(ICanvas&) const override {}
    };
    screens.setFactory([this](ScreenId id) -> std::unique_ptr<IScreen> {
      requested.push_back(id);
      return std::make_unique<Dummy>();
    });
    context.lastName = "Tester";
  }
  Player& me() {
    return static_cast<Player&>(*const_cast<Dungeon&>(controller.dungeon()).player());
  }
  ScreenId next() {
    screens.applyPending();
    return requested.back();
  }

  GameController controller;
  ScoreBoard board;
  StatsTracker stats;
  std::vector<std::unique_ptr<Achievement>> achievements;
  ScreenManager screens;
  std::vector<ScreenId> requested;
  AppContext context{controller, board, screens, stats, achievements,
                     "", Difficulty::Normal, {}, ""};
};

InputFrame key(Key k) {
  InputFrame f;
  f.pressed.push_back(k);
  return f;
}
RecordingCanvas render(const GameOverScreen& screen) {
  RecordingCanvas canvas;
  screen.draw(canvas);
  return canvas;
}

void testShowsSummary() {
  MemoryStore store;
  Fixture fx(store);
  GameOverScreen screen(fx.context);
  const RecordingCanvas canvas = render(screen);
  const RunSummary s = fx.controller.summary();
  CHECK(canvas.hasText("GAME OVER"));
  CHECK(canvas.hasText("Pemain : Tester"));
  CHECK(canvas.hasText("Skor   : " + std::to_string(s.score)));
  CHECK(canvas.hasText("Lantai : " + std::to_string(s.depth)));
  CHECK(canvas.hasText("Gelar  : "));
  CHECK(canvas.hasText("Papan Skor"));
  CHECK(canvas.hasText("Main Lagi"));
  CHECK(canvas.hasText("Menu"));
  CHECK(canvas.allInside(kW, kH));
}

void testScoreSavedExactlyOnce() {
  MemoryStore store;
  Fixture fx(store);
  GameOverScreen screen(fx.context);
  render(screen);
  render(screen);
  screen.update(InputFrame{});
  CHECK_EQ(store.saves, 1);
  CHECK_EQ(fx.board.top().size(), static_cast<std::size_t>(1));
  CHECK_EQ(fx.board.top()[0].name, std::string("Tester"));
  CHECK(render(screen).hasText("1. Tester"));
}

void testAchievementsAndStats() {
  MemoryStore store;
  Fixture fx(store);
  GameOverScreen none(fx.context);
  CHECK(render(none).hasText("(belum ada, sayang sekali)"));

  fx.achievements.push_back(std::make_unique<AlwaysUnlocked>());
  GameEvent event{EventType::Attacked, "", 0};
  fx.achievements[0]->onEvent(event);
  GameOverScreen some(fx.context);
  const RecordingCanvas canvas = render(some);
  CHECK(canvas.hasText("* Selalu Terbuka"));
  CHECK(!canvas.hasText("(belum ada"));
  CHECK(canvas.hasText("Langkah"));
  CHECK(canvas.hasText("Kali menabrak tembok"));
}

void testButtons() {
  {
    MemoryStore store;
    Fixture fx(store);
    GameOverScreen screen(fx.context);
    fx.me().takeDamage(5);
    screen.update(key(Key::Enter));  // fokus awal: Main Lagi
    CHECK(fx.next() == ScreenId::Game);
    CHECK_EQ(fx.controller.playerHp(), Player::kMaxHp);  // run baru
    CHECK_EQ(fx.controller.playerName(), std::string("Tester"));
  }
  {
    MemoryStore store;
    Fixture fx(store);
    GameOverScreen screen(fx.context);
    screen.update(key(Key::Right));
    screen.update(key(Key::Enter));
    CHECK(fx.next() == ScreenId::Menu);
  }
  {
    MemoryStore store;
    Fixture fx(store);
    GameOverScreen screen(fx.context);
    const RecordingCanvas canvas = render(screen);
    int cx = 0, cy = 0;
    for (const auto& t : canvas.texts())
      if (t.text == "Menu") {
        cx = t.x + t.w / 2;
        cy = t.y + t.h / 2;
      }
    InputFrame click;
    click.clicked = true;
    click.mouseX = cx;
    click.mouseY = cy;
    screen.update(click);
    CHECK(fx.next() == ScreenId::Menu);
  }
}

// Review Focus 2: papan penuh dengan nama 16 karakter tetap muat.
void testFullBoardFits() {
  MemoryStore store;
  for (int i = 0; i < 10; ++i) store.data.push_back({"ABCDEFGHIJKLMNOP", 99999 - i, "Legenda"});
  Fixture fx(store);
  fx.achievements.push_back(std::make_unique<AlwaysUnlocked>());
  fx.achievements[0]->onEvent(GameEvent{EventType::Attacked, "", 0});
  GameOverScreen screen(fx.context);
  const RecordingCanvas canvas = render(screen);
  CHECK(canvas.hasText("10."));
  CHECK(canvas.allInside(kW, kH));
}

// Font asli lebih lebar dari stub 8 px: skor di papan tidak boleh terpotong.
void testBoardKeepsScoreWithWideFont() {
  MemoryStore store;
  for (int i = 0; i < 10; ++i) store.data.push_back({"WWWWWWWWWWWWWWWW", 99999 - i, "Legenda"});
  Fixture fx(store);
  GameOverScreen screen(fx.context);
  RecordingCanvas canvas;
  canvas.charWidth = 16;
  screen.draw(canvas);
  for (int i = 0; i < 10; ++i) CHECK(canvas.hasText(std::to_string(99999 - i)));
  CHECK(canvas.allInside(kW, kH));
}

void testFailingStoreShowsMessage() {
  FailingStore store;
  Fixture fx(store);
  GameOverScreen screen(fx.context);
  const RecordingCanvas canvas = render(screen);
  CHECK(canvas.hasText("! Gagal menyimpan skor."));
  CHECK(canvas.allInside(kW, kH));
}

void testThemedLook() {
  MemoryStore store;
  Fixture fx(store);
  GameOverScreen screen(fx.context);
  const RecordingCanvas canvas = render(screen);
  CHECK(canvas.calls.front().kind == RecordingCanvas::Kind::Gradient);
  CHECK(canvas.calls.front().w == kW && canvas.calls.front().h == kH);
  CHECK(canvas.hasText("Hasil Permainan"));
  CHECK(canvas.hasText("Papan Skor"));
  int bevels = 0, bars = 0;
  for (const auto& c : canvas.calls) {
    if (c.kind == RecordingCanvas::Kind::Bevel) ++bevels;
    if (c.kind == RecordingCanvas::Kind::Gradient && c.h == theme::kTitleBarH) ++bars;
  }
  CHECK_EQ(bevels, 2);  // Main Lagi + Menu
  CHECK_EQ(bars, 2);    // dua jendela
  CHECK(canvas.hasText("GAME OVER"));
  CHECK(canvas.allInside(kW, kH));
}

}  // namespace

int main() {
  testShowsSummary();
  testScoreSavedExactlyOnce();
  testAchievementsAndStats();
  testButtons();
  testFullBoardFits();
  testBoardKeepsScoreWithWideFont();
  testFailingStoreShowsMessage();
  testThemedLook();
  return check_summary();
}
