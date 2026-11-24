#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include <ftxui/component/screen_interactive.hpp>

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
#include "ui/AppContext.hpp"
#include "ui/GameOverScreen.hpp"
#include "ui/GameScreen.hpp"
#include "ui/MenuScreen.hpp"
#include "ui/ScreenManager.hpp"

int main() {
  using namespace rogue;
  try {
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

    ScreenManager screens;
    auto screen = ftxui::ScreenInteractive::Fullscreen();

    AppContext context{controller, board, screens, stats, achievements, "",
                       Difficulty::Normal, screen.ExitLoopClosure(),
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
    music->start();
    screen.Loop(screens.host(context.quit));
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
