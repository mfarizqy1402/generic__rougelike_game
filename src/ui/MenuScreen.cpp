#include "ui/MenuScreen.hpp"

#include <utility>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>

#include "controller/InputValidator.hpp"
#include "core/Exceptions.hpp"

namespace rogue {

MenuScreen::MenuScreen(AppContext& context)
    : context_(context),
      name_(context.lastName),
      difficulty_(static_cast<int>(context.lastDifficulty)) {
  using namespace ftxui;

  InputOption singleLine;
  singleLine.multiline = false;  // Enter tidak menambah baris baru
  Component nameInput = Input(&name_, "Nama (3-16 huruf/angka)", singleLine);
  Component seedInput = Input(&seed_, "Seed (angka, kosong = acak)", singleLine);
  Component levelMenu =
      Menu(&difficultyNames_, &difficulty_, MenuOption::Toggle());

  // Tombol Mulai hilang selama ada galat validasi.
  Component startButton = Button("Mulai", [this] { onStart(); }, ButtonOption::Border());
  Component startMaybe =
      Maybe(startButton, [this] { return validationMessage().empty(); });
  Component quitButton = Button(
      "Keluar", [this] {
        if (context_.quit) context_.quit();
      },
      ButtonOption::Border());
  Component buttons = Container::Horizontal({startMaybe, quitButton});

  Component form =
      Container::Vertical({nameInput, seedInput, levelMenu, buttons});

  root_ = Renderer(form, [this, nameInput, seedInput, levelMenu, buttons] {
    auto labelled = [](const std::string& label, Element field) {
      return hbox({text(label) | size(WIDTH, EQUAL, 11), std::move(field) | flex});
    };

    // Galat validasi (dihitung tiap render) didahulukan atas galat startRun.
    std::string error = validationMessage();
    if (error.empty()) error = message_;

    return vbox({
               text("D U N G E O N   K O N Y O L") | bold | color(Color::Yellow) |
                   hcenter,
               text("Roguelike berbasis giliran. Tembok tidak minta maaf.") |
                   dim | hcenter,
               separator(),
               labelled("Nama", nameInput->Render() | border),
               labelled("Seed", seedInput->Render() | border),
               labelled("Kesulitan", levelMenu->Render()),
               text(""),
               text(error.empty() ? "" : "! " + error) | color(Color::Red),
               buttons->Render() | hcenter,
               separator(),
               context_.musicStatus.empty() ? emptyElement()
                                            : text(context_.musicStatus) | dim | hcenter,
               text("Atas/Bawah: pindah kolom  Kiri/Kanan: kesulitan  Esc: keluar") | dim |
                   hcenter,
           }) |
           border | size(WIDTH, EQUAL, 64) | center;
  });
}

ftxui::Component MenuScreen::component() { return root_; }

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

}  // namespace rogue
