#include "core/FortuneTeller.hpp"

#include <cstddef>
#include <random>

namespace rogue {

namespace {

// Hanya karakter ASCII supaya aman di konsol Windows.
const char* const kFortunes[] = {
    "Hari ini kamu menemukan koin emas. Sayangnya itu cokelat berbentuk koin.",
    "Seseorang sedang memikirkanmu. Mungkin penagih utang.",
    "Dalam waktu dekat kamu akan bertemu tembok. Tembok sangat menantikannya.",
    "Bintang-bintang berkata: jangan percaya pada tangga yang terlalu mengkilap.",
    "Keberuntunganmu hari ini setinggi 3 sentimeter. Hati-hati menginjak anak tangga.",
    "Kamu akan menang besar, asalkan lawanmu menyerah karena bosan.",
    "Ramalan hari ini sedang istirahat. Silakan kembali besok.",
    "Seekor slime akan menyapamu dengan ramah, lalu menagih uang parkir.",
    "Rezekimu mengalir deras, sayangnya alirannya menuju dompet orang lain.",
    "Pet Rock menyimpan rahasia besar. Sayang dia batu, jadi tidak bisa bercerita.",
    "Hindari belok kiri hari ini. Atau kanan. Atau lurus. Diam saja mungkin aman.",
    "Kamu ditakdirkan menjadi legenda, minimal di grup keluargamu.",
};

constexpr std::size_t kFortuneCount = sizeof(kFortunes) / sizeof(kFortunes[0]);

}  // namespace

std::string FortuneTeller::fortune(Rng& rng) const {
  std::uniform_int_distribution<std::size_t> pick(0, kFortuneCount - 1);
  return kFortunes[pick(rng)];
}

}  // namespace rogue
