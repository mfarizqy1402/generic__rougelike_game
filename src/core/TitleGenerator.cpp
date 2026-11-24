#include "core/TitleGenerator.hpp"

#include <cstddef>
#include <random>
#include <vector>

namespace rogue {

namespace {

// Hanya ASCII supaya aman di konsol Windows.
const std::vector<std::string>& tier(int index) {
  static const std::vector<std::string> kTiers[4] = {
      // skor < 50
      {"Penjelajah Kaki Lima", "Tamu Tersesat di Dungeon",
       "Pemula Penuh Harapan"},
      // skor < 150
      {"Pemburu Goblin Paruh Waktu", "Petualang Hemat Energi",
       "Penabrak Tembok Berpengalaman"},
      // skor < 400
      {"Ksatria Selokan", "Penakluk Slime Amatir", "Pendekar Tangga Darurat"},
      // skor >= 400
      {"Legenda Warung Kopi", "Raja Dungeon Versi Diskon",
       "Penguasa Tangga Tak Berujung"},
  };
  return kTiers[index];
}

int tierIndexFor(int score) {
  if (score < 50) return 0;  // termasuk skor negatif
  if (score < 150) return 1;
  if (score < 400) return 2;
  return 3;
}

}  // namespace

std::string TitleGenerator::titleFor(int score, Rng& rng) const {
  const std::vector<std::string>& titles = tier(tierIndexFor(score));
  std::uniform_int_distribution<std::size_t> pick(0, titles.size() - 1);
  return titles[pick(rng)];
}

}  // namespace rogue
