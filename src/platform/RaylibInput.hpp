#pragma once

#include "ui/InputFrame.hpp"

namespace rogue {

// Membaca masukan raylib untuk satu frame (panggil sekali per frame).
// Karakter diketik disaring ke ASCII 32..126; panah dan Backspace ikut
// berulang saat ditahan.
InputFrame readInput();

}  // namespace rogue
