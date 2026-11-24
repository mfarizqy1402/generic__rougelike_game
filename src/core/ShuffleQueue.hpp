#pragma once

#include <cstddef>
#include <deque>
#include <filesystem>
#include <optional>
#include <vector>

#include "core/Rng.hpp"

namespace rogue {

// Antrian lagu acak. Diacak sekali saat dibuat; tiap next() mengambil lagu
// terdepan lalu menyisipkannya kembali di posisi acak di antara lagu yang
// tersisa (tidak pernah di depan), sehingga lagu yang sudah diputar masuk lagi
// ke antrian tanpa pernah diulang langsung bila ada lagu lain.
// Memakai Rng sendiri, terpisah dari Rng game.
class ShuffleQueue {
 public:
  ShuffleQueue(std::vector<std::filesystem::path> items, Rng rng);

  std::optional<std::filesystem::path> next();
  std::size_t size() const { return items_.size(); }

 private:
  std::deque<std::filesystem::path> items_;
  Rng rng_;
};

}  // namespace rogue
