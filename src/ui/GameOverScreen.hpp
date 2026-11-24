#pragma once

#include <string>
#include <vector>

#include <ftxui/component/component_base.hpp>

#include "core/ScoreBoard.hpp"
#include "ui/AppContext.hpp"
#include "ui/IScreen.hpp"

namespace rogue {

// Layar akhir run: skor, gelar, achievement, statistik, dan papan skor.
// Skor disimpan sekali, saat layar dibuat.
class GameOverScreen : public IScreen {
 public:
  explicit GameOverScreen(AppContext& context);

  ftxui::Component component() override;

 private:
  void saveScore();
  void onPlayAgain();

  AppContext& context_;
  std::string title_;
  std::string message_;
  std::vector<ScoreEntry> top_;
  ftxui::Component root_;
};

}  // namespace rogue
