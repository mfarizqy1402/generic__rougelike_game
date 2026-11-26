#pragma once

#include <cstdint>
#include <string>

namespace rogue {

struct Rgb {
  std::uint8_t r, g, b;
  bool operator==(const Rgb& o) const { return r == o.r && g == o.g && b == o.b; }
};

// INTERFACE kanvas gambar 2D. Layar hanya tahu antarmuka ini, bukan raylib
// (Dependency Inversion); tes memakai kanvas palsu yang mencatat panggilan.
class ICanvas {
 public:
  virtual ~ICanvas() = default;
  virtual void fillRect(int x, int y, int w, int h, Rgb color) = 0;
  virtual void strokeRect(int x, int y, int w, int h, Rgb color) = 0;
  // Gradasi vertikal: baris atas berwarna `top`, baris bawah `bottom` (efek kilap).
  virtual void gradientRect(int x, int y, int w, int h, Rgb top, Rgb bottom) = 0;
  // Kotak timbul (bevel): isi `face`, tepi atas/kiri `light`, tepi bawah/kanan `dark`.
  virtual void bevelRect(int x, int y, int w, int h, Rgb face, Rgb light, Rgb dark) = 0;
  // (x, y) = pojok kiri atas teks.
  virtual void text(const std::string& s, int x, int y, int size, Rgb color) = 0;
  virtual int textWidth(const std::string& s, int size) const = 0;
};

}  // namespace rogue
