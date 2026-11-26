#include "ui/Palette.hpp"

namespace rogue {

// Tema Frutiger Eco: warna alam yang diredam (tanpa neon), dipakai di atas
// panel kaca terang. Dim dipakai untuk dinding sekaligus teks redup.
Rgb colorOf(ColorId id) {
  switch (id) {
    case ColorId::Dim:
      return {110, 138, 146};  // abu-biru batu
    case ColorId::Yellow:
      return {156, 122, 28};  // emas tua
    case ColorId::Cyan:
      return {58, 124, 165};  // biru laut (pemain)
    case ColorId::Red:
      return {181, 71, 58};  // terakota (musuh)
    case ColorId::Brown:
      return {154, 107, 63};  // cokelat tanah
    case ColorId::LightGreen:
      return {78, 143, 74};  // hijau daun
    case ColorId::DarkGreen:
      return {168, 203, 160};  // lumut muda (zona aman)
    case ColorId::Default:
      break;
  }
  return {36, 66, 74};  // tinta teal-slate
}

}  // namespace rogue
