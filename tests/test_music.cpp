// Tes ShuffleQueue, MusicLibrary, dan NullMusicPlayer.
#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "check.hpp"
#include <fstream>
#include "core/IMusicPlayer.hpp"
#include "core/MusicLibrary.hpp"
#include "core/NullMusicPlayer.hpp"
#include "core/Rng.hpp"
#include "core/ShuffleQueue.hpp"

using namespace rogue;
namespace fs = std::filesystem;

namespace {

std::vector<fs::path> songs(int n) {
  std::vector<fs::path> v;
  for (int i = 0; i < n; ++i) v.emplace_back("lagu" + std::to_string(i) + ".mp3");
  return v;
}

std::vector<std::string> drawOrder(unsigned seed, int calls) {
  ShuffleQueue q(songs(5), Rng(seed));
  std::vector<std::string> out;
  for (int i = 0; i < calls; ++i) out.push_back(q.next()->string());
  return out;
}

void testShuffleQueue() {
  {
    ShuffleQueue q(songs(5), Rng(1));
    std::map<std::string, int> seen;
    for (int i = 0; i < 100; ++i) {
      auto s = q.next();
      CHECK(s.has_value());
      ++seen[s->string()];
      CHECK_EQ(q.size(), static_cast<std::size_t>(5));
    }
    CHECK_EQ(seen.size(), static_cast<std::size_t>(5));
  }
  {
    ShuffleQueue q(songs(2), Rng(3));
    std::optional<fs::path> prev;
    for (int i = 0; i < 50; ++i) {
      auto s = q.next();
      if (prev) CHECK(*s != *prev);
      prev = s;
    }
  }
  {
    ShuffleQueue q(songs(1), Rng(3));
    for (int i = 0; i < 5; ++i) CHECK(*q.next() == fs::path("lagu0.mp3"));
  }
  {
    ShuffleQueue q({}, Rng(3));
    CHECK(!q.next().has_value());
    CHECK_EQ(q.size(), static_cast<std::size_t>(0));
  }
  {
    bool differ = false;
    for (unsigned s = 1; s <= 5; ++s)
      if (drawOrder(s, 5) != drawOrder(s + 1, 5)) differ = true;
    CHECK(differ);
  }
  {
    ShuffleQueue q(songs(5), Rng(7));
    std::map<std::string, int> seen;
    for (int i = 0; i < 15; ++i) ++seen[q.next()->string()];
    int most = 0;
    for (auto& kv : seen) most = std::max(most, kv.second);
    CHECK(most >= 2);
  }
}

void touch(const fs::path& p) {
  fs::create_directories(p.parent_path());
  std::ofstream(p) << "x";
}

void testMusicLibrary() {
  const fs::path root = fs::temp_directory_path() / "rogue_music_test";
  fs::remove_all(root);
  fs::create_directories(root);
  touch(root / "a.mp3");
  touch(root / "B.WAV");
  touch(root / "sub" / "c.flac");
  touch(root / "sub" / "dalam" / "d.Mp3");
  touch(root / "lagu bagus \xC3\xB1.wav");
  touch(root / "catatan.txt");
  touch(root / "gambar.png");
  touch(root / "sub" / "e.ogg");

  MusicLibrary lib(root);
  const auto files = lib.scan();
  CHECK_EQ(files.size(), static_cast<std::size_t>(5));
  CHECK(std::is_sorted(files.begin(), files.end()));
  CHECK(files == lib.scan());
  for (const auto& f : files) {
    const std::string e = f.extension().string();
    CHECK(e != ".txt" && e != ".png" && e != ".ogg");
  }

  CHECK(MusicLibrary(root / "tidak-ada").scan().empty());
  CHECK(MusicLibrary(root / "a.mp3").scan().empty());
  fs::create_directories(root / "kosong");
  CHECK(MusicLibrary(root / "kosong").scan().empty());

  CHECK(MusicLibrary::isMusicFile("x.MP3"));
  CHECK(MusicLibrary::isMusicFile("x.flac"));
  CHECK(!MusicLibrary::isMusicFile("x.mp3.txt"));
  CHECK(!MusicLibrary::isMusicFile("mp3"));

  CHECK_EQ(MusicLibrary::statusText(0), std::string("Musik: tidak ada (isi folder music/)"));
  CHECK_EQ(MusicLibrary::statusText(3), std::string("Musik: 3 lagu"));
  fs::remove_all(root);
}

void testNullMusicPlayer() {
  NullMusicPlayer p;
  IMusicPlayer& i = p;
  i.start();
  CHECK(i.nowPlaying().empty());
  i.stop();
  i.stop();
}

}  // namespace

int main() {
  testShuffleQueue();
  testMusicLibrary();
  testNullMusicPlayer();
  return check_summary();
}
