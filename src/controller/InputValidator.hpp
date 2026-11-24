#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace rogue {

// Validasi input pemain. Semua anggotanya statis (tidak perlu objek).
// Dipanggil dari lapisan UI maupun GameController; galat berupa
// InvalidInputException dengan pesan berbahasa Indonesia.
struct InputValidator {
  // Membuang spasi, tab, dan baris baru di kedua ujung.
  static std::string trimName(const std::string& raw);

  // Hanya huruf ASCII, angka, dan spasi; panjang setelah trim 3..16.
  // Mengembalikan nama yang sudah di-trim.
  static std::string validateName(const std::string& raw);

  // Kosong = std::nullopt. Selain itu hanya digit, maksimal 9 digit.
  static std::optional<std::uint32_t> parseSeed(const std::string& text);
};

}  // namespace rogue
