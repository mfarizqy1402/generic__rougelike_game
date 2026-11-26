#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include <raylib.h>

#include "audio/MiniaudioPlayer.hpp"
#include "controller/GameController.hpp"
#include "core/Achievement.hpp"
#include "core/Exceptions.hpp"
#include "core/FileScoreStore.hpp"
#include "core/IMusicPlayer.hpp"
#include "core/MusicLibrary.hpp"
#include "core/NullMusicPlayer.hpp"
#include "core/Pacifist.hpp"
#include "core/Rng.hpp"
#include "core/ScoreBoard.hpp"
#include "core/ShuffleQueue.hpp"
#include "core/SlimeVictim.hpp"
#include "core/StatsTracker.hpp"
#include "core/WallBumper.hpp"
#include "platform/Display.hpp"
#include "platform/RaylibCanvas.hpp"
#include "platform/RaylibInput.hpp"
#include "ui/AppContext.hpp"
#include "ui/GameOverScreen.hpp"
#include "ui/GameScreen.hpp"
#include "ui/MenuScreen.hpp"
#include "ui/ScreenManager.hpp"

namespace {

constexpr int kWindowW = 960;
constexpr int kWindowH = 680;

// Menutup jendela dengan aman, juga bila terjadi pengecualian.
class WindowGuard {
 public:
  ~WindowGuard() {
    if (IsWindowReady()) CloseWindow();
  }
};

}  // namespace

int main() {
  using namespace rogue;
  try {
    SetTraceLogLevel(LOG_WARNING);
    if (!displayAvailable()) {
      std::cerr << "Galat: jendela grafis tidak bisa dibuat (cek display/driver GL).\n";
      return 1;
    }
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(kWindowW, kWindowH, "generic_roguelike_game");
    if (!IsWindowReady()) {
      std::cerr << "Galat: jendela grafis tidak bisa dibuat (cek display/driver GL).\n";
      return 1;
    }
    WindowGuard windowGuard;
    SetExitKey(KEY_NULL);  // Esc ditangani aplikasi, bukan menutup jendela otomatis
    SetTargetFPS(60);

    GameController controller;
    FileScoreStore store("scores.txt");
    ScoreBoard board(store);
    StatsTracker stats;

    std::vector<std::unique_ptr<Achievement>> achievements;
    achievements.push_back(std::make_unique<WallBumper>());
    achievements.push_back(std::make_unique<SlimeVictim>());
    achievements.push_back(std::make_unique<Pacifist>());

    controller.addObserver(&stats);
    for (auto& a : achievements) controller.addObserver(a.get());

    // Musik latar: semua lagu di folder music/ (termasuk subfolder), diacak.
    const auto songs = MusicLibrary("music").scan();
    std::unique_ptr<IMusicPlayer> music;
    if (songs.empty()) {
      music = std::make_unique<NullMusicPlayer>();
    } else {
      music = std::make_unique<MiniaudioPlayer>(
          ShuffleQueue(songs, Rng(std::random_device{}())));
    }

    bool quitRequested = false;
    ScreenManager screens;
    AppContext context{controller, board, screens, stats, achievements, "",
                       Difficulty::Normal, [&quitRequested] { quitRequested = true; },
                       MusicLibrary::statusText(songs.size())};

    screens.setFactory([&context](ScreenId id) -> std::unique_ptr<IScreen> {
      switch (id) {
        case ScreenId::Menu:
          return std::make_unique<MenuScreen>(context);
        case ScreenId::Game:
          return std::make_unique<GameScreen>(context);
        case ScreenId::GameOver:
          return std::make_unique<GameOverScreen>(context);
      }
      return nullptr;
    });

    screens.go(ScreenId::Menu);
    RaylibCanvas canvas;
    music->start();
    while (!WindowShouldClose() && !quitRequested) {
      screens.update(readInput(), context.quit);
      BeginDrawing();
      ClearBackground(BLACK);
      screens.draw(canvas);
      EndDrawing();
    }
    music->stop();
  } catch (const GameException& error) {
    std::cerr << "Galat game: " << error.what() << '\n';
    return 1;
  } catch (const std::exception& error) {
    std::cerr << "Galat tak terduga: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
