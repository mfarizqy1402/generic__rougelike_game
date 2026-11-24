#pragma once

#include <ftxui/component/component_base.hpp>

namespace rogue {

// INTERFACE sebuah layar TUI. Setiap layar menyediakan satu komponen FTXUI
// yang dipasang ScreenManager sebagai isi jendela.
class IScreen {
 public:
  virtual ~IScreen() = default;
  virtual ftxui::Component component() = 0;
};

}  // namespace rogue
