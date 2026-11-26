#include "ui/GameOverScreen.hpp"

#include <random>
#include <string>

#include "core/Exceptions.hpp"
#include "core/Rng.hpp"
#include "core/TitleGenerator.hpp"
#include "ui/Palette.hpp"
#include "ui/Theme.hpp"

namespace rogue {

namespace {

constexpr int kLeftX = 40;
constexpr int kLeftW = 540;
constexpr int kRightX = 600;
constexpr int kRightW = 320;
constexpr int kPanelY = 22;
constexpr int kPanelH = 500;
constexpr int kWindowW = 960;
constexpr int kWindowH = 680;
constexpr int kLine = 26;
constexpr int kTextSize = 20;
constexpr int kSmallSize = 16;

constexpr int kButtonY = 560;
constexpr int kButtonW = 170;
constexpr int kButtonH = 44;
constexpr int kAgainX = 300;
constexpr int kMenuX = 490;

}  // namespace

GameOverScreen::GameOverScreen(AppContext& context) : context_(context) { saveScore(); }

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

void GameOverScreen::activate(Choice choice) {
  if (choice == Choice::Menu) {
    context_.screens.go(ScreenId::Menu);
    return;
  }
  try {
    context_.controller.startRun(context_.lastName, "", context_.lastDifficulty);
    context_.screens.go(ScreenId::Game);
  } catch (const GameException& error) {
    message_ = error.what();
  }
}

void GameOverScreen::update(const InputFrame& input) {
  for (Key key : input.pressed) {
    switch (key) {
      case Key::Left:
      case Key::Right:
      case Key::Tab:
        focus_ = focus_ == Choice::PlayAgain ? Choice::Menu : Choice::PlayAgain;
        break;
      case Key::Enter:
        activate(focus_);
        break;
      default:
        break;
    }
  }
  if (input.clicked) {
    if (theme::inRect(input.mouseX, input.mouseY, kAgainX, kButtonY, kButtonW, kButtonH)) {
      activate(Choice::PlayAgain);
    } else if (theme::inRect(input.mouseX, input.mouseY, kMenuX, kButtonY, kButtonW, kButtonH)) {
      activate(Choice::Menu);
    }
  }
}

void GameOverScreen::draw(ICanvas& canvas) const {
  const RunSummary s = context_.controller.summary();
  const Rgb white = colorOf(ColorId::Default);  // tinta utama
  const Rgb dim = colorOf(ColorId::Dim);
  const Rgb yellow = colorOf(ColorId::Yellow);
  const int leftInner = kLeftW - 40;
  const int rightInner = kRightW - 40;

  // ---- Panel kiri ----
  theme::background(canvas, kWindowW, kWindowH);
  theme::window(canvas, kLeftX, kPanelY, kLeftW, kPanelH, "Hasil Permainan");
  int x = kLeftX + 20;
  int y = kPanelY + theme::kTitleBarH + 6;
  const std::string over = "GAME OVER";
  canvas.text(over, kLeftX + (kLeftW - canvas.textWidth(over, 32)) / 2, y, 32,
              colorOf(ColorId::Red));
  y += 52;
  canvas.text(theme::fit(canvas, "Pemain : " + context_.controller.playerName(), kTextSize, leftInner),
              x, y, kTextSize, white);
  y += kLine;
  canvas.text("Skor   : " + std::to_string(s.score), x, y, kTextSize, white);
  y += kLine;
  canvas.text("Lantai : " + std::to_string(s.depth), x, y, kTextSize, white);
  y += kLine;
  canvas.text(theme::fit(canvas, "Gelar  : " + title_, kTextSize, leftInner), x, y, kTextSize, yellow);
  y += kLine + 10;

  canvas.text("Achievement:", x, y, kTextSize, white);
  y += kLine;
  bool any = false;
  for (const auto& a : context_.achievements) {
    if (!a->unlocked()) continue;
    any = true;
    canvas.text(theme::fit(canvas, "  * " + a->title(), kTextSize, leftInner), x, y, kTextSize, white);
    y += kLine;
  }
  if (!any) {
    canvas.text("  (belum ada, sayang sekali)", x, y, kTextSize, dim);
    y += kLine;
  }
  y += 10;

  canvas.text("Statistik:", x, y, kTextSize, white);
  y += kLine;
  const struct {
    const char* label;
    const char* key;
  } stats[] = {{"  Langkah              : ", "langkah"},
               {"  Jarak (meter fiktif) : ", "meter_fiktif"},
               {"  Kali ragu-ragu       : ", "ragu_ragu"},
               {"  Kali menabrak tembok : ", "tabrak_tembok"}};
  for (const auto& st : stats) {
    canvas.text(theme::fit(canvas, st.label + std::to_string(context_.stats.get(st.key)), kTextSize,
                    leftInner),
                x, y, kTextSize, white);
    y += kLine;
  }

  // ---- Panel kanan: papan skor ----
  theme::window(canvas, kRightX, kPanelY, kRightW, kPanelH, "Papan Skor");
  int ry = kPanelY + theme::kTitleBarH + 14;
  if (top_.empty()) canvas.text("(kosong)", kRightX + 20, ry, kTextSize, dim);
  for (std::size_t i = 0; i < top_.size(); ++i) {
    // Skor selalu utuh dan rata kanan; hanya nama yang dipotong bila sempit.
    const std::string score = std::to_string(top_[i].score);
    const int scoreW = canvas.textWidth(score, kTextSize);
    canvas.text(theme::fit(canvas, std::to_string(i + 1) + ". " + top_[i].name, kTextSize,
                    rightInner - scoreW - 12),
                kRightX + 20, ry, kTextSize, white);
    canvas.text(score, kRightX + kRightW - 20 - scoreW, ry, kTextSize, white);
    ry += kLine;
  }

  // ---- Pesan galat, tombol, bantuan ----
  if (!message_.empty()) {
    canvas.text(theme::fit(canvas, "! " + message_, kTextSize, 880), kLeftX, kPanelY + kPanelH + 14,
                kTextSize, colorOf(ColorId::Red));
  }
  theme::button(canvas, kAgainX, kButtonY, kButtonW, kButtonH, "Main Lagi", kTextSize,
                focus_ == Choice::PlayAgain);
  theme::button(canvas, kMenuX, kButtonY, kButtonW, kButtonH, "Menu", kTextSize,
                focus_ == Choice::Menu);

  const std::string help = "Kiri/Kanan: pindah tombol   Enter: pilih   Esc: keluar";
  canvas.text(help, (960 - canvas.textWidth(help, kSmallSize)) / 2, 630, kSmallSize, white);
}

}  // namespace rogue
