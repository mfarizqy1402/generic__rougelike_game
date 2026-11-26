// Tes lapisan raylib. Butuh display (jalankan di bawah xvfb-run); tanpa
// display tes dilewati (SKIP) supaya ctest tetap hijau di mesin headless.
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#include <raylib.h>

#include "check.hpp"
#include "controller/GameController.hpp"
#include "core/Achievement.hpp"
#include "core/FileScoreStore.hpp"
#include "core/ScoreBoard.hpp"
#include "core/StatsTracker.hpp"
#include "platform/RaylibCanvas.hpp"
#include "platform/RaylibInput.hpp"
#include "ui/AppContext.hpp"
#include "ui/GameOverScreen.hpp"
#include "ui/GameScreen.hpp"
#include "ui/MenuScreen.hpp"
#include "ui/ScreenManager.hpp"

using namespace rogue;

int main() {
  if (std::getenv("DISPLAY") == nullptr && std::getenv("WAYLAND_DISPLAY") == nullptr) {
    std::cout << "SKIP: tidak ada display (jalankan dengan xvfb-run)\n";
    return 0;
  }

  SetTraceLogLevel(LOG_WARNING);
  SetConfigFlags(FLAG_WINDOW_HIDDEN);
  InitWindow(960, 680, "generic_roguelike_game");
  CHECK(IsWindowReady());
  if (!IsWindowReady()) return check_summary();

  RaylibCanvas canvas;
  CHECK(canvas.textWidth("ABC", 20) > 0);
  CHECK(canvas.textWidth("ABCDEF", 20) > canvas.textWidth("ABC", 20));

  GameController controller;
  FileScoreStore store("test_platform_scores.txt");
  ScoreBoard board(store);
  StatsTracker stats;
  std::vector<std::unique_ptr<Achievement>> achievements;
  ScreenManager screens;
  AppContext context{controller, board, screens, stats, achievements,
                     "", Difficulty::Normal, [] {}, "Musik: 1 lagu"};
  controller.startRun("Tester", "7", Difficulty::Mudah);

  MenuScreen menu(context);
  GameScreen game(context);
  GameOverScreen over(context);
  const IScreen* all[] = {&menu, &game, &over};
  for (const IScreen* screen : all) {
    BeginDrawing();
    ClearBackground(BLACK);
    screen->draw(canvas);
    EndDrawing();
  }

  const InputFrame input = readInput();  // tanpa input nyata: kosong, tidak crash
  CHECK(input.typed.empty());
  CHECK(!input.clicked);

  CloseWindow();
  return check_summary();
}
