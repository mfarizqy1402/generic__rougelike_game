#include "ui/ScreenManager.hpp"

#include <string>
#include <utility>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

namespace rogue {

namespace {

constexpr int kMinColumns = 80;
constexpr int kMinRows = 24;

// Komponen akar: meneruskan render dan event ke layar aktif.
class HostComponent : public ftxui::ComponentBase {
 public:
  HostComponent(ScreenManager& screens, std::function<void()> onEscape)
      : screens_(screens), onEscape_(std::move(onEscape)) {}

  ftxui::Element OnRender() override {
    syncWithManager();

    const ftxui::Dimensions size = ftxui::Terminal::Size();
    if (size.dimx < kMinColumns || size.dimy < kMinRows) {
      return ftxui::vbox({
                 ftxui::text("Perbesar terminal ke minimal 80x24"),
                 ftxui::text("Ukuran sekarang: " + std::to_string(size.dimx) +
                             "x" + std::to_string(size.dimy)),
             }) |
             ftxui::center;
    }
    if (ChildCount() == 0) return ftxui::text("");
    return ChildAt(0)->Render();
  }

  bool OnEvent(ftxui::Event event) override {
    // Esc selalu keluar, di layar mana pun.
    if (event == ftxui::Event::Escape) {
      if (onEscape_) onEscape_();
      return true;
    }
    syncWithManager();

    const ftxui::Dimensions size = ftxui::Terminal::Size();
    if (size.dimx < kMinColumns || size.dimy < kMinRows) return false;
    if (ChildCount() == 0) return false;
    return ChildAt(0)->OnEvent(event);
  }

 private:
  // Dipanggil hanya di awal render/event, tidak pernah dari dalam handler layar.
  void syncWithManager() {
    if (!screens_.applyPending()) return;
    DetachAllChildren();
    if (screens_.current() != nullptr) Add(screens_.current()->component());
  }

  ScreenManager& screens_;
  std::function<void()> onEscape_;
};

}  // namespace

void ScreenManager::setFactory(Factory factory) { factory_ = std::move(factory); }

void ScreenManager::go(ScreenId id) { pending_ = id; }

bool ScreenManager::applyPending() {
  if (!pending_.has_value()) return false;
  const ScreenId id = *pending_;
  pending_.reset();
  if (!factory_) return false;

  std::unique_ptr<IScreen> next = factory_(id);
  if (!next) return false;

  retired_ = std::move(current_);
  current_ = std::move(next);
  return true;
}

IScreen* ScreenManager::current() const { return current_.get(); }

ftxui::Component ScreenManager::host(std::function<void()> onEscape) {
  return std::make_shared<HostComponent>(*this, std::move(onEscape));
}

}  // namespace rogue
