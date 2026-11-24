#include "core/MusicLibrary.hpp"

#include <algorithm>
#include <cctype>
#include <system_error>
#include <utility>

namespace rogue {

namespace fs = std::filesystem;

MusicLibrary::MusicLibrary(fs::path dir) : dir_(std::move(dir)) {}

bool MusicLibrary::isMusicFile(const fs::path& p) {
  std::string ext = p.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return ext == ".mp3" || ext == ".wav" || ext == ".flac";
}

std::vector<fs::path> MusicLibrary::scan() const {
  std::vector<fs::path> result;
  std::error_code ec;
  if (!fs::is_directory(dir_, ec) || ec) return result;

  fs::recursive_directory_iterator it(dir_, fs::directory_options::skip_permission_denied, ec);
  const fs::recursive_directory_iterator end;
  while (!ec && it != end) {
    std::error_code fileEc;
    if (it->is_regular_file(fileEc) && !fileEc && isMusicFile(it->path())) {
      result.push_back(it->path());
    }
    it.increment(ec);
  }
  std::sort(result.begin(), result.end());
  return result;
}

std::string MusicLibrary::statusText(std::size_t songCount) {
  if (songCount == 0) return "Musik: tidak ada (isi folder music/)";
  return "Musik: " + std::to_string(songCount) + " lagu";
}

}  // namespace rogue
