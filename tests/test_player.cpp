// Tes MiniaudioPlayer memakai backend "null" (tanpa perangkat suara nyata).
#include <chrono>
#include <ctime>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "audio/MiniaudioPlayer.hpp"
#include "check.hpp"
#include "core/Rng.hpp"
#include "core/ShuffleQueue.hpp"

using namespace rogue;
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;

namespace {

// WAV PCM 16-bit mono 8000 Hz berisi senyap.
void writeWav(const fs::path& path, double seconds) {
  const std::uint32_t rate = 8000;
  const std::uint32_t samples = static_cast<std::uint32_t>(seconds * rate);
  const std::uint32_t dataBytes = samples * 2;
  auto w32 = [](std::ofstream& f, std::uint32_t v) { f.write(reinterpret_cast<char*>(&v), 4); };
  auto w16 = [](std::ofstream& f, std::uint16_t v) { f.write(reinterpret_cast<char*>(&v), 2); };
  std::ofstream f(path, std::ios::binary);
  f.write("RIFF", 4);
  w32(f, 36 + dataBytes);
  f.write("WAVEfmt ", 8);
  w32(f, 16);
  w16(f, 1);
  w16(f, 1);
  w32(f, rate);
  w32(f, rate * 2);
  w16(f, 2);
  w16(f, 16);
  f.write("data", 4);
  w32(f, dataBytes);
  std::vector<char> zeros(dataBytes, 0);
  f.write(zeros.data(), static_cast<std::streamsize>(zeros.size()));
}

void writeGarbage(const fs::path& path) {
  std::ofstream f(path, std::ios::binary);
  const char junk[] = "ini bukan audio sama sekali \x01\x02\x03\xFF\xFE";
  f.write(junk, sizeof(junk));
}

bool waitFor(const std::function<bool()>& pred, std::chrono::milliseconds timeout) {
  const auto end = Clock::now() + timeout;
  while (Clock::now() < end) {
    if (pred()) return true;
    std::this_thread::sleep_for(20ms);
  }
  return pred();
}

struct TempDir {
  fs::path path;
  explicit TempDir(const std::string& name) : path(fs::temp_directory_path() / name) {
    fs::remove_all(path);
    fs::create_directories(path);
  }
  ~TempDir() { fs::remove_all(path); }
};

MiniaudioOptions nullBackend() {
  MiniaudioOptions o;
  o.useNullBackend = true;
  return o;
}

double secondsSince(Clock::time_point t) {
  return std::chrono::duration<double>(Clock::now() - t).count();
}

void testPlaysAndAdvances() {
  TempDir d("rogue_player_adv");
  writeWav(d.path / "satu.wav", 0.3);
  writeWav(d.path / "dua.wav", 0.3);
  MiniaudioPlayer p(ShuffleQueue({d.path / "satu.wav", d.path / "dua.wav"}, Rng(5)),
                    nullBackend());
  p.start();
  CHECK(waitFor([&] { return !p.nowPlaying().empty(); }, 3000ms));
  std::set<std::string> seen;
  std::string firstName = p.nowPlaying();
  int firstCount = 0;
  const auto end = Clock::now() + 8000ms;
  std::string last;
  while (Clock::now() < end && !(seen.size() >= 2 && firstCount >= 2)) {
    const std::string n = p.nowPlaying();
    if (!n.empty() && n != last) {
      seen.insert(n);
      if (n == firstName) ++firstCount;
      last = n;
    }
    std::this_thread::sleep_for(20ms);
  }
  CHECK_EQ(seen.size(), static_cast<std::size_t>(2));
  CHECK(firstCount >= 2);  // lagu yang sama kembali (disisipkan ulang)
  p.stop();
}

void testSkipsCorruptFile() {
  TempDir d("rogue_player_bad");
  writeGarbage(d.path / "rusak.wav");
  writeWav(d.path / "bagus.wav", 1.0);
  MiniaudioPlayer p(ShuffleQueue({d.path / "rusak.wav", d.path / "bagus.wav"}, Rng(2)),
                    nullBackend());
  p.start();
  CHECK(waitFor([&] { return p.nowPlaying() == "bagus.wav"; }, 4000ms));
  p.stop();
}

// Satu lagu bagus di antara banyak berkas rusak harus selalu ketemu,
// berapa pun urutan acaknya.
void testGoodFileAmongManyCorruptIsAlwaysFound() {
  TempDir d("rogue_player_many_bad");
  std::vector<fs::path> files;
  for (int i = 0; i < 4; ++i) {
    files.push_back(d.path / ("rusak" + std::to_string(i) + ".wav"));
    writeGarbage(files.back());
  }
  writeWav(d.path / "bagus.wav", 1.0);
  files.push_back(d.path / "bagus.wav");
  for (unsigned seed = 0; seed < 20; ++seed) {
    MiniaudioPlayer p(ShuffleQueue(files, Rng(seed)), nullBackend());
    p.start();
    CHECK(waitFor([&] { return p.nowPlaying() == "bagus.wav"; }, 3000ms));
    p.stop();
  }
}

void testAllCorruptDoesNotHang() {
  TempDir d("rogue_player_allbad");
  writeGarbage(d.path / "a.wav");
  writeGarbage(d.path / "b.wav");
  MiniaudioPlayer p(ShuffleQueue({d.path / "a.wav", d.path / "b.wav"}, Rng(2)),
                    nullBackend());
  const auto t0 = Clock::now();
  const std::clock_t cpu0 = std::clock();
  p.start();
  CHECK(secondsSince(t0) < 1.0);
  std::this_thread::sleep_for(1000ms);
  // Tidak boleh berputar terus membuka berkas rusak (CPU hampir nol).
  CHECK(static_cast<double>(std::clock() - cpu0) / CLOCKS_PER_SEC < 0.3);
  CHECK(p.nowPlaying().empty());
  const auto t1 = Clock::now();
  p.stop();
  CHECK(secondsSince(t1) < 1.0);
}

void testStopIsFast() {
  TempDir d("rogue_player_stop");
  writeWav(d.path / "panjang.wav", 3.0);
  MiniaudioPlayer p(ShuffleQueue({d.path / "panjang.wav"}, Rng(1)), nullBackend());
  p.start();
  CHECK(waitFor([&] { return !p.nowPlaying().empty(); }, 3000ms));
  const auto t = Clock::now();
  p.stop();
  CHECK(secondsSince(t) < 1.0);
  CHECK(p.nowPlaying().empty());
  p.stop();  // kedua kali aman
}

void testStartTwiceIsSafe() {
  TempDir d("rogue_player_twice");
  writeWav(d.path / "a.wav", 1.0);
  MiniaudioPlayer p(ShuffleQueue({d.path / "a.wav"}, Rng(1)), nullBackend());
  p.start();
  p.start();
  CHECK(waitFor([&] { return !p.nowPlaying().empty(); }, 3000ms));
  p.stop();
}

void testUnicodeAndSpaces() {
  TempDir d("rogue_player_uni");
  const fs::path name = fs::u8path(u8"lagu bagus ñ.wav");
  writeWav(d.path / name, 1.0);
  MiniaudioPlayer p(ShuffleQueue({d.path / name}, Rng(1)), nullBackend());
  p.start();
  CHECK(waitFor([&] { return !p.nowPlaying().empty(); }, 3000ms));
  CHECK(p.nowPlaying() == name.string());
  p.stop();
}

void testNoDeviceDegradesSilently() {
  TempDir d("rogue_player_nodev");
  writeWav(d.path / "a.wav", 1.0);
  MiniaudioPlayer p(ShuffleQueue({d.path / "a.wav"}, Rng(1)));  // opsi bawaan
  p.start();
  std::this_thread::sleep_for(300ms);
  const auto t = Clock::now();
  p.stop();
  CHECK(secondsSince(t) < 1.0);
}

void testDestructorStops() {
  TempDir d("rogue_player_dtor");
  writeWav(d.path / "a.wav", 3.0);
  const auto t = Clock::now();
  {
    MiniaudioPlayer p(ShuffleQueue({d.path / "a.wav"}, Rng(1)), nullBackend());
    p.start();
    waitFor([&] { return !p.nowPlaying().empty(); }, 3000ms);
  }
  CHECK(secondsSince(t) < 3.0);
}

}  // namespace

int main() {
  testPlaysAndAdvances();
  testSkipsCorruptFile();
  testGoodFileAmongManyCorruptIsAlwaysFound();
  testAllCorruptDoesNotHang();
  testStopIsFast();
  testStartTwiceIsSafe();
  testUnicodeAndSpaces();
  testNoDeviceDegradesSilently();
  testDestructorStops();
  return check_summary();
}
