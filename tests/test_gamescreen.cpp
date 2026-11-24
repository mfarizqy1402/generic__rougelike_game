// Menguji tampilan GameScreen ke layar 80x24 semu (tanpa terminal asli).
#include <memory>
#include <string>
#include <vector>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include "check.hpp"
#include "controller/GameController.hpp"
#include "core/Achievement.hpp"
#include "core/FileScoreStore.hpp"
#include "core/Hiccups.hpp"
#include "core/Pacifist.hpp"
#include "core/Player.hpp"
#include "core/Potion.hpp"
#include "core/ScoreBoard.hpp"
#include "core/StatsTracker.hpp"
#include "ui/AppContext.hpp"
#include "ui/GameScreen.hpp"
#include "ui/MenuScreen.hpp"
#include "ui/ScreenManager.hpp"

using namespace rogue;

namespace {

std::string renderScreen(GameScreen& screen) {
  auto image = ftxui::Screen::Create(ftxui::Dimension::Fixed(80), ftxui::Dimension::Fixed(24));
  ftxui::Render(image, screen.component()->Render());
  return image.ToString();
}

// Tas penuh (9 item) dan satu status aktif: HUD tetap harus menampilkan
// kesembilan item, plus legenda dan baris bantuan di dasar layar.
void testHudShowsFullInventory() {
  GameController controller;
  FileScoreStore store("test_gamescreen_scores.txt");
  ScoreBoard board(store);
  StatsTracker stats;
  std::vector<std::unique_ptr<Achievement>> achievements;
  ScreenManager screens;
  AppContext context{controller, board, screens, stats, achievements, "", Difficulty::Normal, {}, ""};

  controller.startRun("Tester", "7", Difficulty::Mudah);
  Player& me = static_cast<Player&>(*const_cast<Dungeon&>(controller.dungeon()).player());
  while (me.inventory().size() < 9) me.inventory().push_back(std::make_shared<Potion>());
  me.addStatus(std::make_unique<Hiccups>(5));

  GameScreen screen(context);
  const std::string out = renderScreen(screen);
  for (int i = 1; i <= 9; ++i) {
    CHECK(out.find("[" + std::to_string(i) + "]") != std::string::npos);
  }
  CHECK(out.find("Zona aman") != std::string::npos);
  CHECK(out.find("Esc: keluar") != std::string::npos);
}

// Dinding digambar sebagai blok penuh, dan lantai tidak berisi titik.
void testWallsAreBlocksAndFloorIsEmpty() {
  GameController controller;
  FileScoreStore store("test_gamescreen_scores.txt");
  ScoreBoard board(store);
  StatsTracker stats;
  std::vector<std::unique_ptr<Achievement>> achievements;
  ScreenManager screens;
  AppContext context{controller, board, screens, stats, achievements, "", Difficulty::Normal, {}, ""};
  controller.startRun("Tester", "7", Difficulty::Mudah);

  GameScreen screen(context);
  auto image = ftxui::Screen::Create(ftxui::Dimension::Fixed(80), ftxui::Dimension::Fixed(24));
  ftxui::Render(image, screen.component()->Render());

  int blocks = 0;
  for (int y = 1; y <= 15; ++y) {          // isi kotak peta
    for (int x = 1; x <= 51; ++x) {
      const std::string& ch = image.PixelAt(x, y).character;
      if (ch == "\xE2\x96\x88") ++blocks;   // U+2588 blok penuh
      CHECK(ch != ".");
      CHECK(ch != "#");
    }
  }
  CHECK(blocks > 20);
}

// Tiap petak 3 kolom dan simbol berada di kolom tengah (tidak pepet kiri).
void testGlyphsAreCenteredInTheirTile() {
  GameController controller;
  FileScoreStore store("test_gamescreen_scores.txt");
  ScoreBoard board(store);
  StatsTracker stats;
  std::vector<std::unique_ptr<Achievement>> achievements;
  ScreenManager screens;
  AppContext context{controller, board, screens, stats, achievements, "", Difficulty::Normal, {}, ""};
  controller.startRun("Tester", "7", Difficulty::Mudah);

  GameScreen screen(context);
  auto image = ftxui::Screen::Create(ftxui::Dimension::Fixed(80), ftxui::Dimension::Fixed(24));
  ftxui::Render(image, screen.component()->Render());

  int found = 0;
  for (int y = 1; y <= 15; ++y) {
    for (int x = 1; x <= 51; ++x) {
      const std::string& ch = image.PixelAt(x, y).character;
      const bool isSymbol = ch == "@" || ch == "C" || ch == "g" || ch == "s" ||
                            ch == "H" || ch == "o" || ch == ">";
      if (!isSymbol) continue;
      ++found;
      CHECK_EQ((x - 1) % 3, 1);                     // kolom tengah petak 3 kolom
      CHECK(image.PixelAt(x - 1, y).character == " ");
      CHECK(image.PixelAt(x + 1, y).character == " ");
    }
  }
  CHECK(found >= 3);  // minimal '@' dan beberapa peti/musuh terlihat
}

}  // namespace

std::string renderMenu(const std::string& musicStatus) {
  GameController controller;
  FileScoreStore store("test_gamescreen_scores.txt");
  ScoreBoard board(store);
  StatsTracker stats;
  std::vector<std::unique_ptr<Achievement>> achievements;
  ScreenManager screens;
  AppContext context{controller, board, screens, stats, achievements, "", Difficulty::Normal, {}, ""};
  context.musicStatus = musicStatus;
  MenuScreen menu(context);
  auto image = ftxui::Screen::Create(ftxui::Dimension::Fixed(80), ftxui::Dimension::Fixed(24));
  ftxui::Render(image, menu.component()->Render());
  return image.ToString();
}

// Menu menampilkan status musik (satu baris redup), dan tetap muat di 80x24.
void testMenuShowsMusicStatus() {
  const std::string with = renderMenu("Musik: 3 lagu");
  CHECK(with.find("Musik: 3 lagu") != std::string::npos);
  CHECK(with.find("Esc: keluar") != std::string::npos);  // petunjuk tidak terdorong keluar layar
  CHECK(renderMenu("").find("Musik:") == std::string::npos);
}

int main() {
  testHudShowsFullInventory();
  testWallsAreBlocksAndFloorIsEmpty();
  testGlyphsAreCenteredInTheirTile();
  testMenuShowsMusicStatus();
  return check_summary();
}
