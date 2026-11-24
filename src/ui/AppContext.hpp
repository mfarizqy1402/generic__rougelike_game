#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "controller/GameController.hpp"
#include "core/Achievement.hpp"
#include "core/ScoreBoard.hpp"
#include "core/StatsTracker.hpp"
#include "ui/ScreenManager.hpp"

namespace rogue {

// Kumpulan referensi pinjaman yang dibagikan ke semua layar. Tidak memiliki
// objek-objek itu dan tidak berisi logika; main.cpp yang membuat dan
// menjaganya tetap hidup.
struct AppContext {
  GameController& controller;
  ScoreBoard& board;
  ScreenManager& screens;
  StatsTracker& stats;
  const std::vector<std::unique_ptr<Achievement>>& achievements;

  // Pengaturan run terakhir, dipakai tombol "Main Lagi" dan mengisi menu.
  std::string lastName;
  Difficulty lastDifficulty = Difficulty::Normal;

  // Menghentikan aplikasi.
  std::function<void()> quit;

  // Teks status musik untuk menu (kosong = tidak ditampilkan).
  std::string musicStatus;
};

}  // namespace rogue
