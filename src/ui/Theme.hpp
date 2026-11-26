#pragma once

#include <string>

#include "ui/ICanvas.hpp"

namespace rogue {

// Widget bersama bertema Frutiger Eco + Y2K (pemutar media retro). Semua layar
// memakai ini supaya tampilannya seragam dan tak ada logika bisnis di UI.
namespace theme {

inline constexpr int kTitleBarH = 26;

extern const Rgb kSkyTop, kSkyBottom;        // latar jendela (langit mutiara)
extern const Rgb kAquaTop, kAquaBottom;      // title bar kilap
extern const Rgb kTitleText;                 // teks di title bar
extern const Rgb kGlass, kEdge;              // panel kaca dan tepinya
extern const Rgb kSilver, kSilverLight, kSilverDark;  // tombol perak bevel
extern const Rgb kLeaf;                      // tombol fokus (hijau daun)
extern const Rgb kInk;                       // teks utama

bool inRect(int px, int py, int x, int y, int w, int h);

// Memotong teks (dengan "...") agar muat dalam maxWidth piksel.
std::string fit(const ICanvas& canvas, std::string s, int size, int maxWidth);

void background(ICanvas& canvas, int w, int h);
void panel(ICanvas& canvas, int x, int y, int w, int h);
// Panel + title bar aqua kilap berisi judul dan tombol "[_][o][x]". Isi mulai
// di y + kTitleBarH.
void window(ICanvas& canvas, int x, int y, int w, int h, const std::string& title);
void button(ICanvas& canvas, int x, int y, int w, int h, const std::string& label, int size,
            bool focused);

}  // namespace theme
}  // namespace rogue
