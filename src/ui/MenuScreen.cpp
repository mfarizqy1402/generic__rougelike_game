#include "ui/MenuScreen.hpp"

#include <algorithm>
#include <cstddef>

#include "controller/InputValidator.hpp"
#include "core/Exceptions.hpp"
#include "ui/Palette.hpp"
#include "ui/Theme.hpp"

namespace rogue {

namespace {

constexpr std::size_t kMaxNameChars = 16;
constexpr std::size_t kMaxSeedChars = 9;  // sama dengan batas InputValidator

constexpr int kWindowW = 960;
constexpr int kPanelX = 80;
constexpr int kPanelY = 60;
constexpr int kPanelW = 800;
constexpr int kPanelH = 560;

constexpr int kLabelX = 140;
constexpr int kFieldX = 300;
constexpr int kFieldW = 440;
constexpr int kFieldH = 36;
constexpr int kNameY = 220;
constexpr int kSeedY = 280;
constexpr int kLevelY = 340;
constexpr int kErrorY = 400;
constexpr int kButtonY = 440;
constexpr int kButtonW = 140;
constexpr int kButtonH = 40;
constexpr int kStartX = 340;
constexpr int kQuitX = 500;
constexpr int kLevelOptionW = 110;

constexpr int kTitleSize = 26;
constexpr int kTextSize = 20;
constexpr int kSmallSize = 16;

const char* const kDifficultyNames[] = {"Mudah", "Normal", "Sulit"};

void centered(ICanvas& canvas, const std::string& s, int y, int size, Rgb color) {
  const std::string shown = theme::fit(canvas, s, size, kWindowW - 2 * kPanelX - 20);
  canvas.text(shown, (kWindowW - canvas.textWidth(shown, size)) / 2, y, size, color);
}

}  // namespace

MenuScreen::MenuScreen(AppContext& context)
    : context_(context),
      name_(context.lastName),
      difficulty_(static_cast<int>(context.lastDifficulty)) {}

std::vector<MenuScreen::Focus> MenuScreen::focusOrder() const {
  std::vector<Focus> order{Focus::Name, Focus::Seed, Focus::Difficulty};
  if (validationMessage().empty()) order.push_back(Focus::Start);
  order.push_back(Focus::Quit);
  return order;
}

void MenuScreen::moveFocus(int delta) {
  const std::vector<Focus> order = focusOrder();
  const int n = static_cast<int>(order.size());
  int index = 0;
  for (int i = 0; i < n; ++i)
    if (order[static_cast<std::size_t>(i)] == focus_) index = i;
  focus_ = order[static_cast<std::size_t>(((index + delta) % n + n) % n)];
}

void MenuScreen::type(char ch) {
  if (focus_ == Focus::Name) {
    const bool ok = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                    (ch >= '0' && ch <= '9') || ch == ' ';
    if (ok && name_.size() < kMaxNameChars) name_ += ch;
  } else if (focus_ == Focus::Seed) {
    if (ch >= '0' && ch <= '9' && seed_.size() < kMaxSeedChars) seed_ += ch;
  }
}

void MenuScreen::activate(Focus target) {
  if (target == Focus::Start) {
    if (validationMessage().empty()) onStart();
  } else if (target == Focus::Quit) {
    if (context_.quit) context_.quit();
  }
}

void MenuScreen::update(const InputFrame& input) {
  for (Key key : input.pressed) {
    switch (key) {
      case Key::Tab:
      case Key::Down:
        moveFocus(1);
        break;
      case Key::Up:
        moveFocus(-1);
        break;
      case Key::Left:
        if (focus_ == Focus::Difficulty && difficulty_ > 0) --difficulty_;
        break;
      case Key::Right:
        if (focus_ == Focus::Difficulty && difficulty_ < 2) ++difficulty_;
        break;
      case Key::Backspace:
        if (focus_ == Focus::Name && !name_.empty()) name_.pop_back();
        if (focus_ == Focus::Seed && !seed_.empty()) seed_.pop_back();
        break;
      case Key::Enter:
        if (focus_ == Focus::Start || focus_ == Focus::Quit) {
          activate(focus_);
        } else {
          moveFocus(1);
        }
        break;
      case Key::Escape:
        break;  // ditangani ScreenManager
    }
  }

  for (char ch : input.typed) type(ch);

  if (input.clicked) {
    const int mx = input.mouseX;
    const int my = input.mouseY;
    if (theme::inRect(mx, my, kFieldX, kNameY, kFieldW, kFieldH)) focus_ = Focus::Name;
    if (theme::inRect(mx, my, kFieldX, kSeedY, kFieldW, kFieldH)) focus_ = Focus::Seed;
    for (int i = 0; i < 3; ++i) {
      if (theme::inRect(mx, my, kFieldX + i * kLevelOptionW, kLevelY, kLevelOptionW - 6, kFieldH)) {
        focus_ = Focus::Difficulty;
        difficulty_ = i;
      }
    }
    if (theme::inRect(mx, my, kStartX, kButtonY, kButtonW, kButtonH)) activate(Focus::Start);
    if (theme::inRect(mx, my, kQuitX, kButtonY, kButtonW, kButtonH)) activate(Focus::Quit);
  }
}

std::string MenuScreen::validationMessage() const {
  try {
    InputValidator::validateName(name_);
    InputValidator::parseSeed(seed_);
  } catch (const InvalidInputException& error) {
    return error.what();
  }
  return "";
}

void MenuScreen::onStart() {
  try {
    const Difficulty difficulty = static_cast<Difficulty>(difficulty_);
    context_.controller.startRun(name_, seed_, difficulty);
    context_.lastName = context_.controller.playerName();
    context_.lastDifficulty = difficulty;
    message_.clear();
    context_.screens.go(ScreenId::Game);
  } catch (const GameException& error) {
    message_ = error.what();
  }
}

void MenuScreen::draw(ICanvas& canvas) const {
  const Rgb accent = theme::kAquaBottom;
  const Rgb ink = colorOf(ColorId::Default);
  const Rgb dim = colorOf(ColorId::Dim);

  theme::background(canvas, kWindowW, 680);
  theme::window(canvas, kPanelX, kPanelY, kPanelW, kPanelH, "generic_roguelike_game");
  centered(canvas, "G E N E R I C   R O G U E L I K E   G A M E", kPanelY + 40, kTitleSize, accent);
  centered(canvas, "Roguelike berbasis giliran. Tembok tidak minta maaf.", kPanelY + 88,
           kTextSize, dim);

  auto field = [&](const char* label, int y, const std::string& value, bool focused) {
    canvas.text(label, kLabelX, y + 8, kTextSize, ink);
    canvas.fillRect(kFieldX, y, kFieldW, kFieldH, theme::kSilverLight);
    canvas.strokeRect(kFieldX, y, kFieldW, kFieldH, focused ? accent : theme::kEdge);
    const std::string shown = theme::fit(canvas, value, kTextSize, kFieldW - 24);
    canvas.text(shown, kFieldX + 10, y + 8, kTextSize, ink);
    if (focused) {
      const int caretX = kFieldX + 12 + canvas.textWidth(shown, kTextSize);
      canvas.fillRect(caretX, y + 7, 2, kFieldH - 14, accent);
    }
  };
  field("Nama", kNameY, name_, focus_ == Focus::Name);
  field("Seed", kSeedY, seed_.empty() ? std::string() : seed_, focus_ == Focus::Seed);
  if (seed_.empty() && focus_ != Focus::Seed) {
    canvas.text("kosong = acak", kFieldX + 10, kSeedY + 10, kSmallSize, dim);
  }

  canvas.text("Kesulitan", kLabelX, kLevelY + 8, kTextSize, ink);
  for (int i = 0; i < 3; ++i) {
    const int x = kFieldX + i * kLevelOptionW;
    const bool selected = i == difficulty_;
    if (selected) {
      canvas.bevelRect(x, kLevelY, kLevelOptionW - 6, kFieldH,
                       focus_ == Focus::Difficulty ? theme::kLeaf : theme::kSilver,
                       theme::kSilverLight, theme::kSilverDark);
    }
    canvas.text(kDifficultyNames[i], x + 12, kLevelY + 8, kTextSize, selected ? ink : dim);
  }

  const std::string error = validationMessage().empty() ? message_ : validationMessage();
  if (!error.empty()) {
    centered(canvas, "! " + error, kErrorY, kTextSize, colorOf(ColorId::Red));
  }

  if (validationMessage().empty()) {
    theme::button(canvas, kStartX, kButtonY, kButtonW, kButtonH, "Mulai", kTextSize,
                  focus_ == Focus::Start);
  }
  theme::button(canvas, kQuitX, kButtonY, kButtonW, kButtonH, "Keluar", kTextSize,
                focus_ == Focus::Quit);

  if (!context_.musicStatus.empty()) {
    centered(canvas, context_.musicStatus, kPanelY + kPanelH - 70, kSmallSize, dim);
  }
  centered(canvas, "Tab/Atas/Bawah: pindah kolom  Kiri/Kanan: kesulitan  Enter: pilih  Esc: keluar",
           kPanelY + kPanelH - 40, kSmallSize, dim);
}

}  // namespace rogue
