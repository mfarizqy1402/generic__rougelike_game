#include "ui/GameOverScreen.hpp"

#include <random>
#include <utility>

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>

#include "core/Exceptions.hpp"
#include "core/Rng.hpp"
#include "core/TitleGenerator.hpp"

namespace rogue {

GameOverScreen::GameOverScreen(AppContext& context) : context_(context) {
  using namespace ftxui;

  saveScore();

  Component again =
      Button("Main Lagi", [this] { onPlayAgain(); }, ButtonOption::Border());
  Component menu = Button(
      "Menu", [this] { context_.screens.go(ScreenId::Menu); },
      ButtonOption::Border());
  Component buttons = Container::Horizontal({again, menu});

  root_ = Renderer(buttons, [this, buttons] {
    const RunSummary s = context_.controller.summary();

    Elements left;
    left.push_back(text("GAME OVER") | bold | color(Color::Red) | hcenter);
    left.push_back(text(""));
    left.push_back(text("Pemain : " + context_.controller.playerName()));
    left.push_back(text("Skor   : " + std::to_string(s.score)) | bold);
    left.push_back(text("Lantai : " + std::to_string(s.depth)));
    left.push_back(text("Gelar  : " + title_) | color(Color::Yellow));
    left.push_back(separator());

    left.push_back(text("Achievement:") | bold);
    bool any = false;
    for (const auto& a : context_.achievements) {
      if (a->unlocked()) {
        any = true;
        left.push_back(text("  * " + a->title()));
      }
    }
    if (!any) left.push_back(text("  (belum ada, sayang sekali)") | dim);

    left.push_back(separator());
    left.push_back(text("Statistik:") | bold);
    left.push_back(text("  Langkah              : " +
                        std::to_string(context_.stats.get("langkah"))));
    left.push_back(text("  Jarak (meter fiktif) : " +
                        std::to_string(context_.stats.get("meter_fiktif"))));
    left.push_back(text("  Kali ragu-ragu       : " +
                        std::to_string(context_.stats.get("ragu_ragu"))));
    left.push_back(text("  Kali menabrak tembok : " +
                        std::to_string(context_.stats.get("tabrak_tembok"))));

    Elements right;
    right.push_back(text("Papan Skor") | bold | hcenter);
    right.push_back(separator());
    if (top_.empty()) right.push_back(text("(kosong)") | dim);
    for (std::size_t i = 0; i < top_.size(); ++i) {
      right.push_back(text(std::to_string(i + 1) + ". " + top_[i].name + "  " +
                           std::to_string(top_[i].score)));
    }

    Element footer = message_.empty() ? text("") : text("! " + message_) | color(Color::Red);

    return vbox({
               hbox({vbox(std::move(left)) | flex | border,
                     vbox(std::move(right)) | size(WIDTH, EQUAL, 30) | border}),
               footer,
               buttons->Render() | hcenter,
               text("Kiri/Kanan: pindah tombol   Enter: pilih   Esc: keluar") | dim |
                   hcenter,
           }) |
           center;
  });
}

ftxui::Component GameOverScreen::component() { return root_; }

void GameOverScreen::saveScore() {
  const RunSummary s = context_.controller.summary();
  Rng rng(std::random_device{}());
  title_ = TitleGenerator{}.titleFor(s.score, rng);
  try {
    // reload dulu supaya berkas yang rusak tidak tertimpa diam-diam.
    context_.board.reload();
    context_.board.add(ScoreEntry{context_.controller.playerName(), s.score, title_});
  } catch (const GameException& error) {
    message_ = error.what();
  }
  top_ = context_.board.top();
}

void GameOverScreen::onPlayAgain() {
  try {
    context_.controller.startRun(context_.lastName, "", context_.lastDifficulty);
    context_.screens.go(ScreenId::Game);
  } catch (const GameException& error) {
    message_ = error.what();
  }
}

}  // namespace rogue
