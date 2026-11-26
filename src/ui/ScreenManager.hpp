#pragma once

#include <functional>
#include <memory>
#include <optional>

#include "ui/IScreen.hpp"

namespace rogue {

enum class ScreenId { Menu, Game, GameOver };

// Mengelola layar yang sedang tampil. go() hanya MENJADWALKAN pergantian,
// sehingga aman dipanggil dari dalam update() layar itu sendiri; layar baru
// baru dibuat di titik aman (awal update berikutnya atau sesudah update).
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

  // Satu langkah per frame: menerapkan pergantian tertunda, memanggil
  // onEscape bila Esc ditekan (Esc tidak diteruskan ke layar), selain itu
  // meneruskan masukan ke layar aktif lalu menerapkan pergantian baru.
  void update(const InputFrame& input, const std::function<void()>& onEscape);

  // Menggambar layar aktif (tidak melakukan apa pun bila belum ada).
  void draw(ICanvas& canvas) const;

 private:
  Factory factory_;
  std::unique_ptr<IScreen> current_;
  // Layar sebelumnya dipertahankan satu siklus lagi supaya update() yang
  // memanggil go() tidak menghancurkan objeknya sendiri saat masih berjalan.
  std::unique_ptr<IScreen> retired_;
  std::optional<ScreenId> pending_;
};

}  // namespace rogue
