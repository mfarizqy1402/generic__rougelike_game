// Tes IScoreStore, FileScoreStore, dan ScoreBoard.
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "check.hpp"
#include "core/Exceptions.hpp"
#include "core/FileScoreStore.hpp"
#include "core/IScoreStore.hpp"
#include "core/ScoreBoard.hpp"

using namespace rogue;
namespace fs = std::filesystem;

namespace {

// Penyimpanan di memori yang menghitung berapa kali disentuh.
class MemoryStore : public IScoreStore {
 public:
  std::vector<ScoreEntry> load() override {
    ++loads;
    return data;
  }
  void save(const std::vector<ScoreEntry>& entries) override {
    ++saves;
    data = entries;
  }

  std::vector<ScoreEntry> data;
  int loads = 0;
  int saves = 0;
};

// Penyimpanan yang selalu gagal menyimpan.
class FailingStore : public IScoreStore {
 public:
  std::vector<ScoreEntry> load() override { return {}; }
  void save(const std::vector<ScoreEntry>&) override {
    throw ScoreFileException("disk penuh");
  }
};

bool sameEntry(const ScoreEntry& a, const ScoreEntry& b) {
  return a.name == b.name && a.score == b.score && a.title == b.title;
}

bool sameEntries(const std::vector<ScoreEntry>& a,
                 const std::vector<ScoreEntry>& b) {
  if (a.size() != b.size()) return false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (!sameEntry(a[i], b[i])) return false;
  }
  return true;
}

fs::path tempFile(const std::string& name) {
  return fs::temp_directory_path() / ("rogue_uas_test_" + name);
}

void writeRaw(const fs::path& path, const std::string& content) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out << content;
}

// --- ScoreBoard -------------------------------------------------------------

void testConstructorDoesNotTouchStore() {
  MemoryStore store;
  store.data = {ScoreEntry{"Budi", 10, "Gelar"}};
  ScoreBoard board(store);
  CHECK_EQ(store.loads, 0);
  CHECK_EQ(store.saves, 0);
  CHECK(board.top().empty());  // belum reload
}

void testReloadLoadsEntries() {
  MemoryStore store;
  store.data = {ScoreEntry{"Sari", 200, "Gelar A"}, ScoreEntry{"Budi", 100, "Gelar B"}};
  ScoreBoard board(store);
  board.reload();
  CHECK_EQ(store.loads, 1);
  CHECK_EQ(store.saves, 0);
  CHECK_EQ(board.top().size(), static_cast<std::size_t>(2));
  if (board.top().size() == 2) {
    CHECK_EQ(board.top()[0].name, std::string("Sari"));
  }

  // Data di penyimpanan yang tidak urut atau lebih dari 10 tetap dirapikan.
  MemoryStore messy;
  for (int i = 1; i <= 12; ++i) {
    messy.data.push_back(ScoreEntry{"P" + std::to_string(i), (i * 7) % 13, "G"});
  }
  ScoreBoard messyBoard(messy);
  messyBoard.reload();
  CHECK_EQ(messyBoard.top().size(), ScoreBoard::kMaxEntries);
  bool sorted = true;
  for (std::size_t i = 1; i < messyBoard.top().size(); ++i) {
    if (messyBoard.top()[i - 1].score < messyBoard.top()[i].score) sorted = false;
  }
  CHECK(sorted);
}

// Banyak skor seri sekaligus (lewat reload): urutan asli harus dipertahankan.
// Dengan 30 elemen, std::sort biasa (tidak stable) akan mengacak urutan seri.
void testReloadKeepsOrderOfTies() {
  MemoryStore store;
  for (int i = 0; i < 30; ++i) {
    store.data.push_back(ScoreEntry{"T" + std::to_string(i), 5, "G"});
  }
  ScoreBoard board(store);
  board.reload();
  CHECK_EQ(board.top().size(), ScoreBoard::kMaxEntries);
  bool inOriginalOrder = board.top().size() == ScoreBoard::kMaxEntries;
  for (std::size_t i = 0; inOriginalOrder && i < board.top().size(); ++i) {
    if (board.top()[i].name != "T" + std::to_string(i)) inOriginalOrder = false;
  }
  CHECK(inOriginalOrder);
}

void testAddSortsTruncatesAndSaves() {
  MemoryStore store;
  ScoreBoard board(store);

  // 15 skor acak urutannya: hanya 10 tertinggi yang bertahan, urut menurun.
  const int scores[15] = {5, 90, 20, 70, 1, 60, 30, 100, 10, 80, 40, 50, 15, 25, 35};
  for (int i = 0; i < 15; ++i) {
    board.add(ScoreEntry{"Pemain " + std::to_string(i), scores[i], "G"});
  }
  CHECK_EQ(store.saves, 15);  // setiap add menyimpan
  CHECK_EQ(board.top().size(), ScoreBoard::kMaxEntries);
  CHECK_EQ(board.top().front().score, 100);
  CHECK_EQ(board.top().back().score, 25);  // 10 tertinggi: 100..25
  bool sorted = true;
  for (std::size_t i = 1; i < board.top().size(); ++i) {
    if (board.top()[i - 1].score < board.top()[i].score) sorted = false;
  }
  CHECK(sorted);

  // Yang tersimpan sama dengan yang tampil.
  CHECK(sameEntries(store.data, board.top()));
}

void testTiesKeepOlderFirst() {
  MemoryStore store;
  ScoreBoard board(store);
  board.add(ScoreEntry{"Pertama", 50, "G"});
  board.add(ScoreEntry{"Kedua", 50, "G"});
  board.add(ScoreEntry{"Ketiga", 70, "G"});
  CHECK_EQ(board.top().size(), static_cast<std::size_t>(3));
  if (board.top().size() != 3) return;
  CHECK_EQ(board.top()[0].name, std::string("Ketiga"));
  CHECK_EQ(board.top()[1].name, std::string("Pertama"));  // stable
  CHECK_EQ(board.top()[2].name, std::string("Kedua"));
}

void testAddKeepsEntryEvenWhenSaveFails() {
  FailingStore store;
  ScoreBoard board(store);
  CHECK_THROWS(board.add(ScoreEntry{"Budi", 10, "G"}), ScoreFileException);
  // Papan skor di memori tetap berisi, supaya layar bisa menampilkannya.
  CHECK_EQ(board.top().size(), static_cast<std::size_t>(1));
  CHECK_EQ(board.top()[0].name, std::string("Budi"));
}

// --- FileScoreStore ---------------------------------------------------------

void testFileRoundTrip() {
  const fs::path path = tempFile("roundtrip.txt");
  fs::remove(path);

  {
    FileScoreStore store(path.string());
    ScoreBoard board(store);
    board.add(ScoreEntry{"Budi Santoso", 120, "Penjelajah Kaki Lima"});
    board.add(ScoreEntry{"Sari", 300, "Ksatria Selokan"});
    board.add(ScoreEntry{"Dewi", 120, "Gelar dengan spasi"});
  }

  FileScoreStore reopened(path.string());
  ScoreBoard board(reopened);
  board.reload();
  CHECK_EQ(board.top().size(), static_cast<std::size_t>(3));
  if (board.top().size() != 3) {
    fs::remove(path);
    return;
  }
  CHECK_EQ(board.top()[0].name, std::string("Sari"));
  CHECK_EQ(board.top()[0].score, 300);
  CHECK_EQ(board.top()[0].title, std::string("Ksatria Selokan"));
  CHECK_EQ(board.top()[1].name, std::string("Budi Santoso"));
  CHECK_EQ(board.top()[2].name, std::string("Dewi"));

  // Format berkas: satu baris "skor|nama|gelar".
  std::ifstream in(path);
  std::string firstLine;
  std::getline(in, firstLine);
  CHECK_EQ(firstLine, std::string("300|Sari|Ksatria Selokan"));

  fs::remove(path);
}

void testMissingFileMeansEmpty() {
  const fs::path path = tempFile("tidak_ada.txt");
  fs::remove(path);
  FileScoreStore store(path.string());
  CHECK(store.load().empty());

  ScoreBoard board(store);
  board.reload();
  CHECK(board.top().empty());
}

void testCorruptLinesThrow() {
  const fs::path path = tempFile("rusak.txt");
  FileScoreStore store(path.string());

  writeRaw(path, "100|Budi|Gelar\nini baris rusak tanpa pemisah\n");
  CHECK_THROWS(store.load(), ScoreFileException);

  writeRaw(path, "abc|Budi|Gelar\n");  // skor bukan angka
  CHECK_THROWS(store.load(), ScoreFileException);

  writeRaw(path, "12x|Budi|Gelar\n");
  CHECK_THROWS(store.load(), ScoreFileException);

  writeRaw(path, "100|Budi\n");  // hanya satu pemisah
  CHECK_THROWS(store.load(), ScoreFileException);

  writeRaw(path, "|Budi|Gelar\n");  // skor kosong
  CHECK_THROWS(store.load(), ScoreFileException);

  writeRaw(path, "99999999999999999999|Budi|Gelar\n");  // terlalu besar
  CHECK_THROWS(store.load(), ScoreFileException);

  // Baris kosong dan akhir baris CRLF masih sah.
  writeRaw(path, "100|Budi|Gelar\r\n\r\n50|Sari|Lain\r\n");
  const std::vector<ScoreEntry> entries = store.load();
  CHECK_EQ(entries.size(), static_cast<std::size_t>(2));
  if (entries.size() == 2) {
    CHECK_EQ(entries[0].title, std::string("Gelar"));
    CHECK_EQ(entries[1].score, 50);
  }

  // Skor negatif tetap bisa dibaca.
  writeRaw(path, "-5|Budi|Gelar\n");
  const std::vector<ScoreEntry> negative = store.load();
  CHECK_EQ(negative.size(), static_cast<std::size_t>(1));
  if (!negative.empty()) CHECK_EQ(negative[0].score, -5);

  fs::remove(path);
}

void testCorruptFileDoesNotCrashBoardReload() {
  const fs::path path = tempFile("rusak2.txt");
  writeRaw(path, "rusak\n");
  FileScoreStore store(path.string());
  ScoreBoard board(store);
  board.add(ScoreEntry{"Budi", 1, "G"});  // add tidak memuat; hanya menyimpan
  fs::remove(path);

  writeRaw(path, "rusak\n");
  ScoreBoard other(store);
  CHECK_THROWS(other.reload(), ScoreFileException);
  CHECK(other.top().empty());  // gagal memuat: isi lama (kosong) tidak berubah
  fs::remove(path);
}

void testUnwritablePathThrows() {
  const fs::path dir = tempFile("folder_tidak_ada");
  fs::remove_all(dir);
  const fs::path path = dir / "scores.txt";

  FileScoreStore store(path.string());
  const std::vector<ScoreEntry> entries = {ScoreEntry{"Budi", 1, "G"}};
  CHECK_THROWS(store.save(entries), ScoreFileException);

  ScoreBoard board(store);
  CHECK_THROWS(board.add(ScoreEntry{"Budi", 1, "G"}), ScoreFileException);
}

void testDirectoryAsFileThrows() {
  const fs::path dir = tempFile("sebuah_folder");
  fs::remove_all(dir);
  fs::create_directory(dir);
  FileScoreStore store(dir.string());
  CHECK_THROWS(store.load(), ScoreFileException);
  CHECK_THROWS(store.save({ScoreEntry{"Budi", 1, "G"}}), ScoreFileException);
  fs::remove_all(dir);
}

void testEntriesThatWouldCorruptFileAreRejected() {
  const fs::path path = tempFile("tolak.txt");
  fs::remove(path);
  FileScoreStore store(path.string());

  CHECK_THROWS(store.save({ScoreEntry{"a|b", 1, "G"}}), ScoreFileException);
  CHECK_THROWS(store.save({ScoreEntry{"Budi", 1, "dua\nbaris"}}),
               ScoreFileException);
  CHECK_THROWS(store.save({ScoreEntry{"Bu\ndi", 1, "G"}}), ScoreFileException);

  // Gelar boleh mengandung '|' karena sisa baris dianggap gelar.
  store.save({ScoreEntry{"Budi", 7, "A|B"}});
  const std::vector<ScoreEntry> loaded = store.load();
  CHECK_EQ(loaded.size(), static_cast<std::size_t>(1));
  if (!loaded.empty()) CHECK_EQ(loaded[0].title, std::string("A|B"));

  fs::remove(path);
}

}  // namespace

int main() {
  testConstructorDoesNotTouchStore();
  testReloadLoadsEntries();
  testReloadKeepsOrderOfTies();
  testAddSortsTruncatesAndSaves();
  testTiesKeepOlderFirst();
  testAddKeepsEntryEvenWhenSaveFails();
  testFileRoundTrip();
  testMissingFileMeansEmpty();
  testCorruptLinesThrow();
  testCorruptFileDoesNotCrashBoardReload();
  testUnwritablePathThrows();
  testDirectoryAsFileThrows();
  testEntriesThatWouldCorruptFileAreRejected();
  return check_summary();
}
