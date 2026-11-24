#include "controller/InputValidator.hpp"

#include <cstddef>

#include "core/Exceptions.hpp"

namespace rogue {

namespace {

constexpr std::size_t kMinNameLength = 3;
constexpr std::size_t kMaxNameLength = 16;
constexpr std::size_t kMaxSeedDigits = 9;

bool isTrimChar(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// Perbandingan manual agar tidak bergantung pada locale dan menolak non-ASCII.
bool isNameChar(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == ' ';
}

bool isDigit(char c) { return c >= '0' && c <= '9'; }

}  // namespace

std::string InputValidator::trimName(const std::string& raw) {
  std::size_t begin = 0;
  std::size_t end = raw.size();
  while (begin < end && isTrimChar(raw[begin])) ++begin;
  while (end > begin && isTrimChar(raw[end - 1])) --end;
  return raw.substr(begin, end - begin);
}

std::string InputValidator::validateName(const std::string& raw) {
  const std::string name = trimName(raw);
  if (name.size() < kMinNameLength) {
    throw InvalidInputException("Nama minimal 3 karakter.");
  }
  if (name.size() > kMaxNameLength) {
    throw InvalidInputException("Nama maksimal 16 karakter.");
  }
  for (char c : name) {
    if (!isNameChar(c)) {
      throw InvalidInputException(
          "Nama hanya boleh berisi huruf, angka, dan spasi.");
    }
  }
  return name;
}

std::optional<std::uint32_t> InputValidator::parseSeed(const std::string& text) {
  const std::string digits = trimName(text);
  if (digits.empty()) return std::nullopt;

  for (char c : digits) {
    if (!isDigit(c)) {
      throw InvalidInputException("Seed hanya boleh berisi angka.");
    }
  }
  if (digits.size() > kMaxSeedDigits) {
    throw InvalidInputException("Seed maksimal 9 digit.");
  }

  std::uint32_t value = 0;
  for (char c : digits) value = value * 10u + static_cast<std::uint32_t>(c - '0');
  return value;
}

}  // namespace rogue
