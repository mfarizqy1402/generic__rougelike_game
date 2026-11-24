#pragma once

#include <string>

#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "ui/AppContext.hpp"
#include "ui/IScreen.hpp"

namespace rogue {

// Layar permainan: peta di kiri, HUD di kanan, log di bawah, bantuan tombol.
// Event handler hanya menerjemahkan satu tombol menjadi satu panggilan
// controller; semua aturan game ada di GameController.
class GameScreen : public IScreen {
 public:
  explicit GameScreen(AppContext& context);

  ftxui::Component component() override;

 private:
  ftxui::Element render() const;
  ftxui::Element renderMap() const;
  ftxui::Element renderHud() const;
  ftxui::Element renderLog() const;
  ftxui::Element renderLegend() const;

  bool handleEvent(const ftxui::Event& event);

  AppContext& context_;
  std::string message_;  // galat dari controller (mis. item tidak ada)
  ftxui::Component root_;
};

}  // namespace rogue
