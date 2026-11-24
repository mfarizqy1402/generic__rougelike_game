#pragma once

#include <string>
#include <vector>

#include "core/IScoreStore.hpp"

namespace rogue {

// Menyimpan skor ke berkas teks, satu baris per entri: "skor|nama|gelar".
// Berkas tidak ada = daftar kosong. Baris rusak, skor bukan angka, atau gagal
// menulis melempar ScoreFileException.
class FileScoreStore : public IScoreStore {
 public:
  explicit FileScoreStore(std::string path);

  std::vector<ScoreEntry> load() override;
  void save(const std::vector<ScoreEntry>& entries) override;

 private:
  std::string path_;
};

}  // namespace rogue
