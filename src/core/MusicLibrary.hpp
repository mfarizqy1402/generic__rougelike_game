#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace rogue {

// Mencari berkas musik (.mp3 .wav .flac) di sebuah folder, termasuk subfolder.
// Tidak pernah melempar: folder yang tidak ada atau tidak terbaca = daftar kosong.
class MusicLibrary {
 public:
  explicit MusicLibrary(std::filesystem::path dir);

  std::vector<std::filesystem::path> scan() const;

  static bool isMusicFile(const std::filesystem::path& p);
  static std::string statusText(std::size_t songCount);

 private:
  std::filesystem::path dir_;
};

}  // namespace rogue
