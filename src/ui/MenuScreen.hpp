#pragma once

#include <string>
#include <vector>

#include <ftxui/component/component_base.hpp>

#include "ui/AppContext.hpp"
#include "ui/IScreen.hpp"

namespace rogue {

// Layar menu: nama, seed (opsional), kesulitan, tombol Mulai dan Keluar.
class MenuScreen : public IScreen {
 public:
  explicit MenuScreen(AppContext& context);

  ftxui::Component component() override;

 private:
  // Pesan galat validasi; kosong bila input sudah benar. Dihitung ulang
  // setiap render lewat InputValidator.
  std::string validationMessage() const;

  // Handler tombol Mulai: hanya memanggil controller lalu pindah layar.
  void onStart();

  AppContext& context_;
  std::string name_;
  std::string seed_;
  std::string message_;
  int difficulty_ = 1;
  std::vector<std::string> difficultyNames_{"Mudah", "Normal", "Sulit"};
  ftxui::Component root_;
};

}  // namespace rogue
