#pragma once

#include <string>
#include <vector>

#include "ui/AppContext.hpp"
#include "ui/IScreen.hpp"

namespace rogue {

// Layar menu: nama, seed (opsional), kesulitan, tombol Mulai dan Keluar.
class MenuScreen : public IScreen {
 public:
  explicit MenuScreen(AppContext& context);

  void update(const InputFrame& input) override;
  void draw(ICanvas& canvas) const override;

 private:
  enum class Focus { Name, Seed, Difficulty, Start, Quit };

  // Urutan fokus saat ini; Mulai dilewati selama input tidak valid.
  std::vector<Focus> focusOrder() const;
  void moveFocus(int delta);
  void type(char ch);
  void activate(Focus target);

  // Pesan galat validasi; kosong bila input sudah benar.
  std::string validationMessage() const;

  // Tombol Mulai: hanya memanggil controller lalu pindah layar.
  void onStart();

  AppContext& context_;
  std::string name_;
  std::string seed_;
  std::string message_;
  int difficulty_ = 1;
  Focus focus_ = Focus::Name;
};

}  // namespace rogue
