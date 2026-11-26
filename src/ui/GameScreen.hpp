#pragma once

#include <string>

#include "ui/AppContext.hpp"
#include "ui/IScreen.hpp"

namespace rogue {

// Layar permainan: peta di kiri, HUD di kanan, log di bawah, legenda, dan
// baris bantuan tombol. update() hanya menerjemahkan satu tombol menjadi satu
// panggilan controller; semua aturan game ada di GameController.
class GameScreen : public IScreen {
 public:
  explicit GameScreen(AppContext& context);

  void update(const InputFrame& input) override;
  void draw(ICanvas& canvas) const override;

 private:
  enum class Action { Move, Item };
  void perform(Action action, int dirOrIndex);
  void drawMap(ICanvas& canvas) const;
  void drawHud(ICanvas& canvas) const;
  void drawLog(ICanvas& canvas) const;
  void drawLegend(ICanvas& canvas) const;

  AppContext& context_;
  std::string message_;  // galat dari controller (mis. item tidak ada)
  bool finished_ = false;  // Game Over sudah dijadwalkan
};

}  // namespace rogue
