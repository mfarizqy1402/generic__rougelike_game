#pragma once

#include <string>
#include <vector>

#include "core/ScoreBoard.hpp"
#include "ui/AppContext.hpp"
#include "ui/IScreen.hpp"

namespace rogue {

// Layar akhir run: skor, gelar, achievement, statistik, dan papan skor.
// Skor disimpan sekali, saat layar dibuat.
class GameOverScreen : public IScreen {
 public:
  explicit GameOverScreen(AppContext& context);

  void update(const InputFrame& input) override;
  void draw(ICanvas& canvas) const override;

 private:
  enum class Choice { PlayAgain, Menu };

  void saveScore();
  void activate(Choice choice);

  AppContext& context_;
  std::string title_;
  std::string message_;
  std::vector<ScoreEntry> top_;
  Choice focus_ = Choice::PlayAgain;
};

}  // namespace rogue
