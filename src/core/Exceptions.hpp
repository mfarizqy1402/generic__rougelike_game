#pragma once

#include <stdexcept>
#include <string>

namespace rogue {

// Akar semua galat yang berasal dari aturan game.
class GameException : public std::runtime_error {
 public:
  explicit GameException(const std::string& message)
      : std::runtime_error(message) {}
};

// Input pemain tidak valid (nama, seed, dsb.).
class InvalidInputException : public GameException {
 public:
  using GameException::GameException;
};

// Gerakan atau aksi yang tidak diperbolehkan.
class InvalidMoveException : public GameException {
 public:
  using GameException::GameException;
};

// Gagal membaca atau menulis berkas skor.
class ScoreFileException : public GameException {
 public:
  using GameException::GameException;
};

}  // namespace rogue
