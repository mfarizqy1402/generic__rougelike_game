#pragma once

#include <string>

#include "core/Rng.hpp"

namespace rogue {

// Pemberi gelar lucu berdasarkan skor akhir.
// Tingkat: skor < 50, < 150, < 400, dan selebihnya. Skor negatif masuk
// tingkat terendah.
class TitleGenerator {
 public:
  std::string titleFor(int score, Rng& rng) const;
};

}  // namespace rogue
