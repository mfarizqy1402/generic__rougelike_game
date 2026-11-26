// Tes ScreenManager dengan layar palsu (tanpa jendela).
#include <functional>
#include <memory>
#include <string>

#include "RecordingCanvas.hpp"
#include "check.hpp"
#include "ui/IScreen.hpp"
#include "ui/InputFrame.hpp"
#include "ui/ScreenManager.hpp"

using namespace rogue;

namespace {

struct Counters {
  int created = 0;
  int destroyed = 0;
  int updates = 0;
  int draws = 0;
  int sawEscape = 0;
};

// Layar palsu; bila `goTo` diisi, memanggil screens.go() dari dalam update().
class FakeScreen : public IScreen {
 public:
  FakeScreen(Counters& c, ScreenManager& screens, bool goOnUpdate)
      : c_(c), screens_(screens), goOnUpdate_(goOnUpdate) {
    ++c_.created;
  }
  ~FakeScreen() override { ++c_.destroyed; }
  void update(const InputFrame& in) override {
    ++c_.updates;
    if (in.pressedKey(Key::Escape)) ++c_.sawEscape;
    if (goOnUpdate_) {
      screens_.go(ScreenId::Game);
      ++afterGo_;  // dijalankan setelah go(); objek ini harus masih hidup
    }
  }
  void draw(ICanvas& canvas) const override {
    ++c_.draws;
    canvas.text("fake", 0, 0, 20, Rgb{255, 255, 255});
  }

 private:
  Counters& c_;
  ScreenManager& screens_;
  bool goOnUpdate_;
  int afterGo_ = 0;
};

void testGoOnlySchedules() {
  Counters c;
  ScreenManager screens;
  screens.setFactory([&](ScreenId) -> std::unique_ptr<IScreen> {
    return std::make_unique<FakeScreen>(c, screens, false);
  });
  screens.go(ScreenId::Menu);
  CHECK_EQ(c.created, 0);
  CHECK(screens.current() == nullptr);
  CHECK(screens.applyPending());
  CHECK_EQ(c.created, 1);
  CHECK(screens.current() != nullptr);
  CHECK(!screens.applyPending());
}

void testGoFromInsideUpdateIsSafe() {
  Counters c;
  ScreenManager screens;
  screens.setFactory([&](ScreenId id) -> std::unique_ptr<IScreen> {
    return std::make_unique<FakeScreen>(c, screens, id == ScreenId::Menu);
  });
  screens.go(ScreenId::Menu);
  screens.applyPending();
  screens.update(InputFrame{}, [] {});
  // Layar Menu masih hidup selama update()-nya berjalan, lalu diganti.
  CHECK_EQ(c.created, 2);
  CHECK(screens.current() != nullptr);
  CHECK_EQ(c.updates, 1);
}

void testEscapeCallsOnEscapeOnce() {
  Counters c;
  ScreenManager screens;
  screens.setFactory([&](ScreenId) -> std::unique_ptr<IScreen> {
    return std::make_unique<FakeScreen>(c, screens, false);
  });
  screens.go(ScreenId::Menu);
  int escapes = 0;
  InputFrame esc;
  esc.pressed.push_back(Key::Escape);
  screens.update(esc, [&] { ++escapes; });
  CHECK_EQ(escapes, 1);
  CHECK_EQ(c.sawEscape, 0);  // tidak diteruskan ke layar
  InputFrame other;
  other.pressed.push_back(Key::Enter);
  screens.update(other, [&] { ++escapes; });
  CHECK_EQ(escapes, 1);
  CHECK_EQ(c.updates, 1);
}

void testDrawWithoutScreenDoesNotCrash() {
  ScreenManager screens;
  RecordingCanvas canvas;
  screens.draw(canvas);
  CHECK(canvas.calls.empty());
}

void testDrawForwardsToCurrent() {
  Counters c;
  ScreenManager screens;
  screens.setFactory([&](ScreenId) -> std::unique_ptr<IScreen> {
    return std::make_unique<FakeScreen>(c, screens, false);
  });
  screens.go(ScreenId::Menu);
  screens.update(InputFrame{}, [] {});
  RecordingCanvas canvas;
  screens.draw(canvas);
  CHECK_EQ(c.draws, 1);
  CHECK(canvas.hasText("fake"));
}

}  // namespace

int main() {
  testGoOnlySchedules();
  testGoFromInsideUpdateIsSafe();
  testEscapeCallsOnEscapeOnce();
  testDrawWithoutScreenDoesNotCrash();
  testDrawForwardsToCurrent();
  return check_summary();
}
