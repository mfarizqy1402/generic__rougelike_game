#pragma once

#include <cstddef>
#include <vector>

#include "core/IScoreStore.hpp"

namespace rogue {

// Papan skor 10 besar. Bergantung pada antarmuka IScoreStore (bukan kelas
// berkas tertentu), dan hanya meminjam penyimpanannya (aggregation).
class ScoreBoard {
 public:
  static constexpr std::size_t kMaxEntries = 10;

  // Konstruktor TIDAK menyentuh penyimpanan; panggil reload() untuk memuat.
  explicit ScoreBoard(IScoreStore& store);

  // Memuat dari penyimpanan (dirapikan: urut menurun, maksimal 10).
  // Bila gagal, isi lama tidak berubah.
  void reload();

  // Menambah entri, mengurutkan menurun (stable), memotong 10 teratas, lalu
  // menyimpan. Bila penyimpanan gagal, ScoreFileException dilempar tetapi
  // isi di memori tetap sudah diperbarui.
  void add(const ScoreEntry& entry);

  const std::vector<ScoreEntry>& top() const;

 private:
  void normalize();

  IScoreStore& store_;
  std::vector<ScoreEntry> entries_;
};

}  // namespace rogue
