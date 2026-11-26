#pragma once

#include <string>
#include <vector>

#include "controller/GameController.hpp"

namespace rogue {

// Warna logis sebuah petak. Lapisan UI (Palette) yang menerjemahkannya ke warna
// layar; file ini sengaja TANPA include UI/raylib agar bisa diuji.
enum class ColorId {
  Default,
  Dim,         // dinding (UI menggambarnya sebagai blok penuh)
  Yellow,      // tangga
  Cyan,        // pemain
  Red,         // semua musuh (Goblin, Slime, Hantu)
  Brown,       // Pet Rock
  LightGreen,  // peti
  DarkGreen    // lantai zona aman (latar hijau, glyph ' ')
};

struct Cell {
  char glyph = ' ';
  ColorId color = ColorId::Default;
};

// Potongan peta seukuran viewW x viewH petak, berpusat pada pemain dan
// dijepit di tepi peta. Bagian di luar peta (bila viewport lebih besar dari
// peta) diisi spasi. Hasilnya grid[baris][kolom].
//   dinding '#', lantai '.', tangga '>', pemain '@', Goblin 'g',
//   Slime 's', Ghost 'H', Pet Rock 'o', peti 'C'. Musuh selalu merah.
//   Lantai biasa kosong (' ', Default); lantai zona aman juga ' ' tetapi
//   ColorId::DarkGreen (UI menggambarnya sebagai latar hijau).
// Melempar std::invalid_argument bila ukuran <= 0, dan InvalidMoveException
// bila belum ada run.
std::vector<std::vector<Cell>> buildViewport(const GameController& controller,
                                             int viewW, int viewH);

// Satu baris legenda: simbol di peta, warnanya, nama, dan apakah itu musuh.
struct LegendItem {
  char glyph;
  ColorId color;
  std::string label;
  bool enemy;
};

// Daftar legenda tetap: Kamu, Goblin, Slime, Hantu, Batu, Peti, Tangga, Zona aman, Tembok.
std::vector<LegendItem> legend();

}  // namespace rogue
