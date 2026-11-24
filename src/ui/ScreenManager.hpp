#pragma once

#include <functional>
#include <memory>
#include <optional>

#include <ftxui/component/component_base.hpp>

#include "ui/IScreen.hpp"

namespace rogue {

enum class ScreenId { Menu, Game, GameOver };

// Mengelola layar yang sedang tampil. go() hanya MENJADWALKAN pergantian,
// sehingga aman dipanggil dari dalam event handler layar itu sendiri; layar
// baru baru dibuat di titik aman (awal render atau awal event berikutnya).
class ScreenManager {
 public:
  // Pabrik layar. Dipasang main.cpp, karena layar butuh AppContext yang
  // juga memegang referensi ke ScreenManager ini.
  using Factory = std::function<std::unique_ptr<IScreen>(ScreenId)>;

  void setFactory(Factory factory);

  // Menjadwalkan pergantian layar. Tidak membuat atau menghancurkan apa pun.
  void go(ScreenId id);

  // Menjalankan pergantian yang dijadwalkan. True bila layar berganti.
  bool applyPending();

  IScreen* current() const;

  // Komponen akar untuk ScreenInteractive::Loop(): menampilkan layar aktif,
  // menerapkan pergantian layar di titik aman, memeriksa ukuran terminal
  // (minimal 80x24), dan memanggil onEscape saat tombol Esc ditekan.
  ftxui::Component host(std::function<void()> onEscape);

 private:
  Factory factory_;
  std::unique_ptr<IScreen> current_;
  // Layar sebelumnya dipertahankan satu siklus lagi supaya tidak ada callback
  // FTXUI yang sempat memegang pointer ke layar yang sudah hancur.
  std::unique_ptr<IScreen> retired_;
  std::optional<ScreenId> pending_;
};

}  // namespace rogue
