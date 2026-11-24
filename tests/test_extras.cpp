// Tes fitur "tidak berguna": Achievement (+3 konkret), StatsTracker,
// TitleGenerator.
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

#include "check.hpp"
#include "controller/GameController.hpp"
#include "core/Achievement.hpp"
#include "core/Dungeon.hpp"
#include "core/Enemy.hpp"
#include "core/GameEvent.hpp"
#include "core/IGameObserver.hpp"
#include "core/Pacifist.hpp"
#include "core/Rng.hpp"
#include "core/SlimeVictim.hpp"
#include "core/StatsTracker.hpp"
#include "core/TitleGenerator.hpp"
#include "core/Types.hpp"
#include "core/WallBumper.hpp"

using namespace rogue;

// Achievement adalah kelas abstrak sekaligus pengamat; konkretnya bukan abstrak.
static_assert(std::is_abstract_v<Achievement>, "Achievement harus abstrak");
static_assert(std::is_base_of_v<IGameObserver, Achievement>,
              "Achievement harus turunan IGameObserver");
static_assert(!std::is_abstract_v<WallBumper>, "WallBumper harus konkret");
static_assert(!std::is_abstract_v<SlimeVictim>, "SlimeVictim harus konkret");
static_assert(!std::is_abstract_v<Pacifist>, "Pacifist harus konkret");

namespace {

void feed(IGameObserver& observer, EventType type, int times,
          const std::string& detail = "") {
  for (int i = 0; i < times; ++i) observer.onEvent(GameEvent{type, detail, 0});
}

void runStarted(IGameObserver& observer) {
  observer.onEvent(GameEvent{EventType::RunStarted, "Budi", 0});
}

// --- Achievement ------------------------------------------------------------

void testWallBumper() {
  WallBumper a;
  CHECK_EQ(a.title(), std::string("Menabrak tembok 50 kali"));
  CHECK(!a.unlocked());

  feed(a, EventType::WallBump, 49);
  CHECK(!a.unlocked());
  feed(a, EventType::WallBump, 1);
  CHECK(a.unlocked());

  // RunStarted mereset status dan hitungan.
  runStarted(a);
  CHECK(!a.unlocked());
  feed(a, EventType::WallBump, 49);
  CHECK(!a.unlocked());
  feed(a, EventType::WallBump, 1);
  CHECK(a.unlocked());

  // Event lain tidak dihitung.
  WallBumper other;
  feed(other, EventType::Moved, 200);
  feed(other, EventType::Waited, 200, "diam");
  CHECK(!other.unlocked());

  // Hitungan berlaku per run.
  WallBumper perRun;
  feed(perRun, EventType::WallBump, 30);
  runStarted(perRun);
  feed(perRun, EventType::WallBump, 30);
  CHECK(!perRun.unlocked());
}

void testSlimeVictim() {
  SlimeVictim a;
  CHECK_EQ(a.title(), std::string("Mati karena slime"));
  CHECK(!a.unlocked());

  feed(a, EventType::PlayerDied, 1, "Goblin");
  feed(a, EventType::PlayerDied, 1, "Ghost");
  CHECK(!a.unlocked());

  // Event selain PlayerDied dengan detail "Slime" tidak berarti apa-apa.
  feed(a, EventType::Attacked, 5, "Slime");
  CHECK(!a.unlocked());

  feed(a, EventType::PlayerDied, 1, "Slime");
  CHECK(a.unlocked());

  runStarted(a);
  CHECK(!a.unlocked());
}

void testPacifist() {
  Pacifist a;
  CHECK_EQ(a.title(), std::string("Menunggu 20 kali tanpa alasan"));
  CHECK(!a.unlocked());

  // Hanya Waited dengan detail "diam" yang dihitung.
  feed(a, EventType::Waited, 100, "tertahan");
  CHECK(!a.unlocked());

  feed(a, EventType::Waited, 19, "diam");
  CHECK(!a.unlocked());
  feed(a, EventType::Waited, 1, "diam");
  CHECK(a.unlocked());

  runStarted(a);
  CHECK(!a.unlocked());
  feed(a, EventType::Waited, 19, "diam");
  CHECK(!a.unlocked());
}

void testAchievementsThroughBasePointers() {
  std::vector<std::unique_ptr<Achievement>> all;
  all.push_back(std::make_unique<WallBumper>());
  all.push_back(std::make_unique<SlimeVictim>());
  all.push_back(std::make_unique<Pacifist>());

  std::set<std::string> titles;
  for (const auto& a : all) {
    CHECK(!a->title().empty());
    titles.insert(a->title());
  }
  CHECK_EQ(titles.size(), static_cast<std::size_t>(3));

  // Semua menerima event yang sama lewat antarmuka IGameObserver.
  for (const auto& a : all) {
    IGameObserver& observer = *a;
    feed(observer, EventType::WallBump, 50);
    feed(observer, EventType::PlayerDied, 1, "Slime");
    feed(observer, EventType::Waited, 20, "diam");
  }
  for (const auto& a : all) CHECK(a->unlocked());

  for (const auto& a : all) runStarted(*a);
  for (const auto& a : all) CHECK(!a->unlocked());
}

// --- StatsTracker -----------------------------------------------------------

void testStatsTracker() {
  StatsTracker stats;

  // Semua kunci ada sejak awal, bernilai 0.
  CHECK_EQ(stats.stats().size(), static_cast<std::size_t>(4));
  CHECK_EQ(stats.get("langkah"), 0);
  CHECK_EQ(stats.get("meter_fiktif"), 0);
  CHECK_EQ(stats.get("ragu_ragu"), 0);
  CHECK_EQ(stats.get("tabrak_tembok"), 0);
  CHECK_EQ(stats.get("kunci_tidak_ada"), 0);

  feed(stats, EventType::Moved, 2);
  CHECK_EQ(stats.get("langkah"), 2);
  CHECK_EQ(stats.get("meter_fiktif"), 6);  // 3 per langkah

  feed(stats, EventType::Waited, 1, "diam");
  feed(stats, EventType::Waited, 2, "tertahan");
  CHECK_EQ(stats.get("ragu_ragu"), 3);  // semua Waited

  feed(stats, EventType::WallBump, 4);
  CHECK_EQ(stats.get("tabrak_tembok"), 4);

  // Event lain tidak mengubah apa pun.
  feed(stats, EventType::Attacked, 5, "Goblin");
  feed(stats, EventType::EnemyDied, 5, "Goblin");
  CHECK_EQ(stats.get("langkah"), 2);
  CHECK_EQ(stats.get("ragu_ragu"), 3);
  CHECK_EQ(stats.stats().size(), static_cast<std::size_t>(4));

  // RunStarted mereset semuanya.
  runStarted(stats);
  CHECK_EQ(stats.get("langkah"), 0);
  CHECK_EQ(stats.get("meter_fiktif"), 0);
  CHECK_EQ(stats.get("ragu_ragu"), 0);
  CHECK_EQ(stats.get("tabrak_tembok"), 0);
}

// --- TitleGenerator ---------------------------------------------------------

std::set<std::string> titlesFor(int score) {
  const TitleGenerator generator;
  std::set<std::string> titles;
  for (std::uint32_t seed = 0; seed < 300; ++seed) {
    Rng rng(seed);
    titles.insert(generator.titleFor(score, rng));
  }
  return titles;
}

bool disjoint(const std::set<std::string>& a, const std::set<std::string>& b) {
  for (const auto& title : a) {
    if (b.count(title) != 0) return false;
  }
  return true;
}

void testTitleGenerator() {
  const TitleGenerator generator;

  // Tidak pernah kosong, termasuk skor ekstrem.
  const int scores[] = {-1, 0, 50, 500, 100000};
  for (int score : scores) {
    Rng rng(1);
    CHECK(!generator.titleFor(score, rng).empty());
  }

  // Deterministik: Rng seed sama memberi gelar sama.
  Rng a(9);
  Rng b(9);
  CHECK_EQ(generator.titleFor(70, a), generator.titleFor(70, b));

  // Tingkat: < 50, < 150, < 400, selebihnya; negatif = terendah.
  const std::set<std::string> low = titlesFor(0);
  const std::set<std::string> mid = titlesFor(50);
  const std::set<std::string> high = titlesFor(150);
  const std::set<std::string> top = titlesFor(400);

  CHECK(low.size() >= 2);
  CHECK(mid.size() >= 2);
  CHECK(high.size() >= 2);
  CHECK(top.size() >= 2);

  CHECK(low.count("Penjelajah Kaki Lima") == 1);
  CHECK(titlesFor(-1) == low);
  CHECK(titlesFor(-500) == low);
  CHECK(titlesFor(49) == low);
  CHECK(titlesFor(149) == mid);
  CHECK(titlesFor(399) == high);
  CHECK(titlesFor(100000) == top);

  CHECK(disjoint(low, mid));
  CHECK(disjoint(mid, high));
  CHECK(disjoint(high, top));
  CHECK(disjoint(low, top));

  // Hanya ASCII.
  bool ascii = true;
  for (const auto* tier : {&low, &mid, &high, &top}) {
    for (const auto& title : *tier) {
      for (char ch : title) {
        if (static_cast<unsigned char>(ch) > 127) ascii = false;
      }
    }
  }
  CHECK(ascii);
}

// --- Integrasi dengan GameController -----------------------------------------

class Counter : public IGameObserver {
 public:
  void onEvent(const GameEvent& event) override {
    if (event.type == EventType::Moved) ++moved;
    if (event.type == EventType::Waited) ++waited;
    if (event.type == EventType::WallBump) ++bumps;
  }
  int moved = 0;
  int waited = 0;
  int bumps = 0;
};

void quiet(GameController& c) {
  Dungeon& d = const_cast<Dungeon&>(c.dungeon());
  for (auto& e : d.entities()) {
    if (dynamic_cast<Enemy*>(e.get()) != nullptr) e->takeDamage(1000);
  }
}

void testObserversWorkWithController() {
  GameController c;
  StatsTracker stats;
  Pacifist pacifist;
  WallBumper bumper;
  SlimeVictim victim;
  Counter counter;
  c.addObserver(&stats);
  c.addObserver(&pacifist);
  c.addObserver(&bumper);
  c.addObserver(&victim);
  c.addObserver(&counter);

  c.startRun("Budi", "15", Difficulty::Mudah);
  quiet(c);

  const Dir script[6] = {Dir::Left, Dir::Left, Dir::Up,
                         Dir::Right, Dir::Down, Dir::None};
  for (int i = 0; i < 150 && !c.isOver(); ++i) c.movePlayer(script[i % 6]);

  // Statistik cocok dengan event yang benar-benar terjadi.
  CHECK(counter.moved > 0);
  CHECK(counter.waited > 0);
  CHECK(counter.bumps > 0);
  CHECK_EQ(stats.get("langkah"), counter.moved);
  CHECK_EQ(stats.get("meter_fiktif"), counter.moved * 3);
  CHECK_EQ(stats.get("ragu_ragu"), counter.waited);
  CHECK_EQ(stats.get("tabrak_tembok"), counter.bumps);
  CHECK_EQ(stats.get("langkah"), c.summary().steps);

  // Menunggu 20 kali membuka Pacifist.
  c.startRun("Budi", "15", Difficulty::Mudah);  // RunStarted: semua reset
  CHECK_EQ(stats.get("langkah"), 0);
  CHECK(!pacifist.unlocked());
  quiet(c);
  for (int i = 0; i < 20; ++i) c.movePlayer(Dir::None);
  CHECK(pacifist.unlocked());
  CHECK(!bumper.unlocked());
  CHECK(!victim.unlocked());

  // Run baru menutup lagi achievement dan menolkan statistik.
  c.startRun("Budi", "15", Difficulty::Mudah);
  CHECK(!pacifist.unlocked());
  CHECK_EQ(stats.get("ragu_ragu"), 0);
}

}  // namespace

int main() {
  testWallBumper();
  testSlimeVictim();
  testPacifist();
  testAchievementsThroughBasePointers();
  testStatsTracker();
  testTitleGenerator();
  testObserversWorkWithController();
  return check_summary();
}
