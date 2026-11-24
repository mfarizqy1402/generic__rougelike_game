#include "core/FileScoreStore.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <utility>

#include "core/Exceptions.hpp"

namespace rogue {

namespace {

constexpr std::size_t kMaxScoreDigits = 9;

// Skor: tanda minus opsional, lalu 1..9 digit.
int parseScore(const std::string& text) {
  std::size_t i = 0;
  bool negative = false;
  if (!text.empty() && text[0] == '-') {
    negative = true;
    i = 1;
  }
  const std::size_t digits = text.size() - i;
  if (digits == 0 || digits > kMaxScoreDigits) {
    throw ScoreFileException("Berkas skor rusak: skor tidak valid.");
  }
  int value = 0;
  for (; i < text.size(); ++i) {
    if (text[i] < '0' || text[i] > '9') {
      throw ScoreFileException("Berkas skor rusak: skor bukan angka.");
    }
    value = value * 10 + (text[i] - '0');
  }
  return negative ? -value : value;
}

ScoreEntry parseLine(const std::string& line) {
  const std::size_t first = line.find('|');
  const std::size_t second =
      (first == std::string::npos) ? std::string::npos : line.find('|', first + 1);
  if (first == std::string::npos || second == std::string::npos) {
    throw ScoreFileException("Berkas skor rusak: format baris salah.");
  }
  ScoreEntry entry;
  entry.score = parseScore(line.substr(0, first));
  entry.name = line.substr(first + 1, second - first - 1);
  entry.title = line.substr(second + 1);  // sisa baris, boleh memuat '|'
  return entry;
}

bool hasLineBreak(const std::string& text) {
  return text.find('\n') != std::string::npos ||
         text.find('\r') != std::string::npos;
}

}  // namespace

FileScoreStore::FileScoreStore(std::string path) : path_(std::move(path)) {}

std::vector<ScoreEntry> FileScoreStore::load() {
  std::error_code ec;
  const std::filesystem::path path(path_);
  if (!std::filesystem::exists(path, ec)) return {};  // belum ada skor
  if (!std::filesystem::is_regular_file(path, ec)) {
    throw ScoreFileException("Berkas skor tidak bisa dibaca.");
  }

  std::ifstream in(path, std::ios::binary);
  if (!in.is_open()) {
    throw ScoreFileException("Berkas skor tidak bisa dibuka.");
  }

  std::vector<ScoreEntry> entries;
  std::string line;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();  // CRLF
    if (line.empty()) continue;
    entries.push_back(parseLine(line));
  }
  return entries;
}

void FileScoreStore::save(const std::vector<ScoreEntry>& entries) {
  // Tolak entri yang akan merusak format berkas.
  for (const ScoreEntry& entry : entries) {
    if (entry.name.find('|') != std::string::npos || hasLineBreak(entry.name) ||
        hasLineBreak(entry.title)) {
      throw ScoreFileException("Entri skor memuat karakter yang tidak boleh.");
    }
  }

  std::ofstream out(std::filesystem::path(path_), std::ios::binary | std::ios::trunc);
  if (!out.is_open()) {
    throw ScoreFileException("Gagal membuka berkas skor untuk ditulis.");
  }
  for (const ScoreEntry& entry : entries) {
    out << entry.score << '|' << entry.name << '|' << entry.title << '\n';
  }
  out.flush();
  if (!out.good()) {
    throw ScoreFileException("Gagal menulis berkas skor.");
  }
}

}  // namespace rogue
