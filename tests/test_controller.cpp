// Tes InputValidator, FortuneTeller, dan GameController.
//
// Beberapa skenario butuh situasi yang pasti (mis. musuh tepat di sebelah
// pemain). Untuk itu tes mengubah Dungeon lewat const_cast pada
// controller.dungeon(): objeknya sendiri tidak const, jadi ini legal, dan
// hanya dipakai di tes.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "check.hpp"
#include "controller/GameController.hpp"
#include "controller/InputValidator.hpp"
#include "core/Chest.hpp"
#include "core/Dungeon.hpp"
#include "core/Enemy.hpp"
#include "core/Exceptions.hpp"
#include "core/FortuneTeller.hpp"
#include "core/GameEvent.hpp"
#include "core/Goblin.hpp"
#include "core/Hiccups.hpp"
#include "core/IGameObserver.hpp"
#include "core/Overconfident.hpp"
#include "core/PetRock.hpp"
#include "core/Player.hpp"
#include "core/Potion.hpp"
#include "core/Rng.hpp"
#include "core/Scroll.hpp"
#include "core/Tile.hpp"
#include "core/Types.hpp"

using namespace rogue;

namespace {

const Dir kDirs[4] = {Dir::Up, Dir::Down, Dir::Left, Dir::Right};
const std::string kGoblinTaunt = "Goblin: Hehehe, dompetmu kelihatan berat!";

// Pengamat yang mencatat semua event.
class Recorder : public IGameObserver {
 public:
  void onEvent(const GameEvent& event) override { events.push_back(event); }

  int count(EventType type) const {
    int n = 0;
    for (const auto& e : events) {
      if (e.type == type) ++n;
    }
    return n;
  }

  std::vector<GameEvent> events;
};

Dungeon& mutableDungeon(GameController& c) {
  return const_cast<Dungeon&>(c.dungeon());
}

Player& playerOf(GameController& c) {
  return *dynamic_cast<Player*>(mutableDungeon(c).player());
}

// Membuat semua musuh mati supaya skenario tidak terganggu.
void quiet(GameController& c) {
  for (auto& e : mutableDungeon(c).entities()) {
    if (dynamic_cast<Enemy*>(e.get()) != nullptr) e->takeDamage(1000);
  }
}

bool isFree(const Dungeon& d, Pos p) {
  return d.inBounds(p) && d.tile(p).isWalkable() && d.entityAt(p) == nullptr;
}

// Arah dari from ke to (keduanya bertetangga).
Dir directionTo(Pos from, Pos to) {
  if (to.x > from.x) return Dir::Right;
  if (to.x < from.x) return Dir::Left;
  if (to.y > from.y) return Dir::Down;
  return Dir::Up;
}

bool freeNeighborDir(const Dungeon& d, Pos p, Dir& out) {
  for (Dir dir : kDirs) {
    if (isFree(d, step(p, dir))) {
      out = dir;
      return true;
    }
  }
  return false;
}

Pos petRockPos(const Dungeon& d) {
  for (const auto& e : d.entities()) {
    if (dynamic_cast<const PetRock*>(e.get()) != nullptr) return e->pos();
  }
  return Pos{-1, -1};
}

int countEnemies(const Dungeon& d) {
  int n = 0;
  for (const auto& e : d.entities()) {
    if (dynamic_cast<const Enemy*>(e.get()) != nullptr) ++n;
  }
  return n;
}

// Petak walkable (bukan tangga) yang kirinya tembok dan jauh dari Pet Rock.
bool findWallOnLeft(const Dungeon& d, Pos& out) {
  const Pos rock = petRockPos(d);
  for (int y = 0; y < d.height(); ++y) {
    for (int x = 1; x < d.width(); ++x) {
      const Pos p{x, y};
      if (d.tile(p).type() != Floor) continue;
      if (d.tile(step(p, Dir::Left)).isWalkable()) continue;
      if (manhattan(p, rock) < 4) continue;
      if (d.entityAt(p) != nullptr) continue;
      out = p;
      return true;
    }
  }
  return false;
}

// Petak lantai dengan lantai kosong di kiri dan kanannya.
bool findOpenRow(const Dungeon& d, Pos& out) {
  for (int y = 1; y < d.height() - 1; ++y) {
    for (int x = 2; x < d.width() - 2; ++x) {
      const Pos p{x, y};
      const Pos left = step(p, Dir::Left);
      const Pos right = step(p, Dir::Right);
      if (d.tile(p).type() != Floor || d.tile(left).type() != Floor ||
          d.tile(right).type() != Floor) {
        continue;
      }
      if (isFree(d, p) && isFree(d, left) && isFree(d, right)) {
        out = p;
        return true;
      }
    }
  }
  return false;
}

// Memindahkan pemain ke petak kosong di samping tangga; toward = arah ke tangga.
bool placeBesideStairs(GameController& c, Dir& toward) {
  Dungeon& d = mutableDungeon(c);
  const Pos stairs = d.stairs();
  for (Dir e : kDirs) {
    const Pos n = step(stairs, e);
    if (isFree(d, n)) {
      playerOf(c).setPos(n);
      toward = directionTo(n, stairs);
      return true;
    }
  }
  return false;
}

bool logContains(const GameController& c, const std::string& line) {
  const auto& log = c.log();
  return std::find(log.begin(), log.end(), line) != log.end();
}

// ---------------------------------------------------------------------------
// InputValidator
// ---------------------------------------------------------------------------

void expectNameThrows(const std::string& raw) {
  CHECK_THROWS(InputValidator::validateName(raw), InvalidInputException);
}

void testNameValidation() {
  CHECK_EQ(InputValidator::trimName("  x  "), std::string("x"));
  CHECK_EQ(InputValidator::trimName("\t x\n"), std::string("x"));
  CHECK_EQ(InputValidator::trimName("   "), std::string(""));

  CHECK_EQ(InputValidator::validateName("abc"), std::string("abc"));
  CHECK_EQ(InputValidator::validateName("  Budi Santoso "),
           std::string("Budi Santoso"));
  CHECK_EQ(InputValidator::validateName("abcdefghijklmnop"),
           std::string("abcdefghijklmnop"));  // 16 karakter
  CHECK_EQ(InputValidator::validateName("123"), std::string("123"));

  expectNameThrows("");
  expectNameThrows("   ");
  expectNameThrows("ab");
  expectNameThrows("  ab  ");  // panjang dihitung setelah trim
  expectNameThrows("abcdefghijklmnopq");  // 17 karakter
  expectNameThrows("Budi!");
  expectNameThrows("Bud_i");
  expectNameThrows("Bu\tdi");
  expectNameThrows("Budi\xC3\xA9");  // bukan ASCII

  // Pesan galat berbahasa Indonesia dan tidak kosong.
  std::string message;
  try {
    InputValidator::validateName("ab");
  } catch (const InvalidInputException& e) {
    message = e.what();
  }
  CHECK(!message.empty());
}

void expectSeed(const std::string& text, std::uint32_t expected) {
  const std::optional<std::uint32_t> seed = InputValidator::parseSeed(text);
  CHECK(seed.has_value());
  if (seed.has_value()) CHECK_EQ(*seed, expected);
}

void expectSeedThrows(const std::string& text) {
  CHECK_THROWS(InputValidator::parseSeed(text), InvalidInputException);
}

void testSeedParsing() {
  CHECK(!InputValidator::parseSeed("").has_value());
  CHECK(!InputValidator::parseSeed("   ").has_value());
  expectSeed("0", 0u);
  expectSeed("000000001", 1u);
  expectSeed("007", 7u);
  expectSeed(" 42 ", 42u);
  expectSeed("123456789", 123456789u);  // 9 digit

  expectSeedThrows("12a");
  expectSeedThrows("-1");
  expectSeedThrows("+5");
  expectSeedThrows("1 2");
  expectSeedThrows("1234567890");  // 10 digit
}

// ---------------------------------------------------------------------------
// FortuneTeller
// ---------------------------------------------------------------------------

void testFortuneTeller() {
  const FortuneTeller teller;

  Rng a(5);
  Rng b(5);
  CHECK_EQ(teller.fortune(a), teller.fortune(b));

  std::set<std::string> seen;
  bool allAsciiAndNonEmpty = true;
  for (std::uint32_t seed = 0; seed < 1000; ++seed) {
    Rng rng(seed);
    const std::string text = teller.fortune(rng);
    if (text.empty()) allAsciiAndNonEmpty = false;
    for (char ch : text) {
      if (static_cast<unsigned char>(ch) > 127) allAsciiAndNonEmpty = false;
    }
    seen.insert(text);
  }
  CHECK(allAsciiAndNonEmpty);
  CHECK(seen.size() >= 10);  // minimal 10 ramalan berbeda
}

// ---------------------------------------------------------------------------
// GameController
// ---------------------------------------------------------------------------

void testActionsBeforeStartThrow() {
  GameController c;
  CHECK(!c.isOver());
  CHECK(c.log().empty());
  CHECK_EQ(c.fortune(), std::string(""));
  CHECK_THROWS(c.movePlayer(Dir::Up), InvalidMoveException);
  CHECK_THROWS(c.useItem(0), InvalidMoveException);
  CHECK_THROWS(c.dungeon(), InvalidMoveException);
  CHECK_THROWS(c.playerHp(), InvalidMoveException);
  CHECK_THROWS(c.summary(), InvalidMoveException);
}

void testStartRunRejectsBadInput() {
  GameController c;
  Recorder recorder;
  c.addObserver(&recorder);

  CHECK_THROWS(c.startRun("ab", "", Difficulty::Normal), InvalidInputException);
  CHECK_THROWS(c.startRun("Budi!", "", Difficulty::Normal),
               InvalidInputException);
  CHECK_THROWS(c.startRun("Budi", "12a", Difficulty::Normal),
               InvalidInputException);
  CHECK_THROWS(c.startRun("Budi", "1234567890", Difficulty::Normal),
               InvalidInputException);
  CHECK_EQ(recorder.events.size(), static_cast<std::size_t>(0));
  CHECK_THROWS(c.movePlayer(Dir::Up), InvalidMoveException);  // belum mulai

  // Input buruk di tengah run tidak merusak run yang sedang berjalan.
  c.startRun("Budi", "1", Difficulty::Normal);
  CHECK_THROWS(c.startRun("x", "", Difficulty::Normal), InvalidInputException);
  CHECK_EQ(c.playerName(), std::string("Budi"));
  CHECK(!c.isOver());
  CHECK_EQ(recorder.count(EventType::RunStarted), 1);
}

void testRunStartedEventAndPlayerName() {
  GameController c;
  Recorder recorder;
  c.addObserver(&recorder);

  c.startRun("  Budi  ", "", Difficulty::Normal);  // seed kosong: acak
  CHECK_EQ(c.playerName(), std::string("Budi"));
  CHECK_EQ(recorder.events.size(), static_cast<std::size_t>(1));
  CHECK(recorder.events[0].type == EventType::RunStarted);
  CHECK_EQ(recorder.events[0].detail, std::string("Budi"));

  c.startRun("Sari", "3", Difficulty::Mudah);  // pengamat tetap terpasang
  CHECK_EQ(c.playerName(), std::string("Sari"));
  CHECK_EQ(recorder.count(EventType::RunStarted), 2);
}

void testFortuneComesRightAfterSeeding() {
  Rng rng(77);
  const std::string expected = FortuneTeller().fortune(rng);

  GameController c;
  c.startRun("Budi", "77", Difficulty::Normal);
  CHECK_EQ(c.fortune(), expected);
}

void testInitialFloor() {
  const Difficulty levels[] = {Difficulty::Mudah, Difficulty::Normal,
                               Difficulty::Sulit};
  for (int index = 0; index < 3; ++index) {
    for (std::uint32_t seed = 1; seed <= 10; ++seed) {
      GameController c;
      c.startRun("Budi", std::to_string(seed), levels[index]);
      const Dungeon& d = c.dungeon();

      CHECK_EQ(d.width(), 40);
      CHECK_EQ(d.height(), 25);
      CHECK_EQ(c.playerPos(), d.start());
      CHECK_EQ(c.playerHp(), Player::kMaxHp);
      CHECK_EQ(c.depth(), 1);
      CHECK(!c.isOver());
      CHECK(!c.log().empty());
      CHECK_EQ(c.inventoryNames(),
               (std::vector<std::string>{"Ramuan", "Gulungan Oleng"}));
      CHECK(c.statusNames().empty());

      // Jumlah musuh = 2 + lantai + indeks kesulitan.
      CHECK_EQ(countEnemies(d), 2 + 1 + index);

      // Pet Rock menempel di samping start.
      CHECK_EQ(manhattan(petRockPos(d), d.start()), 1);

      // Musuh: lantai biasa, jauh dari start, tidak bertumpuk.
      bool placementOk = true;
      for (const auto& e : d.entities()) {
        if (dynamic_cast<const Enemy*>(e.get()) == nullptr) continue;
        if (manhattan(e->pos(), d.start()) < 5) placementOk = false;
        if (d.tile(e->pos()).type() != Floor) placementOk = false;
        for (const auto& other : d.entities()) {
          if (other.get() != e.get() && other->pos() == e->pos()) {
            placementOk = false;
          }
        }
      }
      CHECK(placementOk);
    }
  }
}

void testWallBumpDoesNotUseTurn() {
  GameController c;
  Recorder recorder;
  c.addObserver(&recorder);
  c.startRun("Budi", "5", Difficulty::Mudah);
  quiet(c);

  Dungeon& d = mutableDungeon(c);
  Pos spot;
  CHECK(findWallOnLeft(d, spot));
  playerOf(c).setPos(spot);
  playerOf(c).addStatus(std::make_unique<Overconfident>(1));
  const Pos rockBefore = petRockPos(d);

  recorder.events.clear();
  c.movePlayer(Dir::Left);

  CHECK_EQ(recorder.events.size(), static_cast<std::size_t>(1));
  CHECK(recorder.events[0].type == EventType::WallBump);
  CHECK_EQ(c.playerPos(), spot);
  CHECK_EQ(c.summary().steps, 0);
  CHECK_EQ(petRockPos(d), rockBefore);             // tidak ada yang bergerak
  CHECK_EQ(c.statusNames().size(), static_cast<std::size_t>(1));  // tidak tick
  CHECK_EQ(c.log().back(),
           std::string("Kamu menabrak tembok. Tembok tidak minta maaf."));

  // Sebaliknya, diam memakai giliran: Pet Rock mendekat, status berkurang.
  recorder.events.clear();
  c.movePlayer(Dir::None);
  CHECK_EQ(recorder.events.size(), static_cast<std::size_t>(1));
  CHECK(recorder.events[0].type == EventType::Waited);
  CHECK_EQ(recorder.events[0].detail, std::string("diam"));
  CHECK(petRockPos(d) != rockBefore);
  CHECK(c.statusNames().empty());  // Overconfident(1) kedaluwarsa
  CHECK_EQ(c.summary().steps, 0);
}

void testHeldBackByStatus() {
  GameController c;
  Recorder recorder;
  c.addObserver(&recorder);
  c.startRun("Budi", "6", Difficulty::Mudah);
  quiet(c);

  Pos spot;
  CHECK(findOpenRow(c.dungeon(), spot));
  playerOf(c).setPos(spot);
  playerOf(c).addStatus(std::make_unique<Overconfident>(5));

  c.movePlayer(Dir::Right);
  const Pos afterFirst = c.playerPos();
  CHECK_EQ(afterFirst, (Pos{spot.x + 1, spot.y}));

  recorder.events.clear();
  c.movePlayer(Dir::Left);  // mundur: ditolak status
  CHECK_EQ(c.playerPos(), afterFirst);
  CHECK_EQ(recorder.events.size(), static_cast<std::size_t>(1));
  CHECK(recorder.events[0].type == EventType::Waited);
  CHECK_EQ(recorder.events[0].detail, std::string("tertahan"));
}

void testNeverLeavesMap() {
  for (Dir dir : kDirs) {
    GameController c;
    c.startRun("Budi", "8", Difficulty::Mudah);
    quiet(c);
    bool inside = true;
    for (int i = 0; i < 100 && !c.isOver(); ++i) {
      c.movePlayer(dir);
      const Dungeon& d = c.dungeon();
      if (!d.inBounds(c.playerPos())) inside = false;
      if (!d.tile(c.playerPos()).isWalkable()) inside = false;
    }
    CHECK(inside);
  }
}

void testAttackKillsEnemyAndTauntIsLogged() {
  GameController c;
  Recorder recorder;
  c.addObserver(&recorder);
  c.startRun("Budi", "9", Difficulty::Mudah);
  quiet(c);

  Dungeon& d = mutableDungeon(c);
  const Pos here = c.playerPos();
  Dir toward = Dir::None;
  CHECK(freeNeighborDir(d, here, toward));
  const Pos goblinPos = step(here, toward);
  d.add(std::make_unique<Goblin>(goblinPos));

  // Serangan pertama: Goblin hp 6 -> 3, lalu Goblin membalas.
  recorder.events.clear();
  c.movePlayer(toward);
  CHECK_EQ(recorder.count(EventType::Attacked), 2);
  CHECK_EQ(recorder.events[0].value, 3);                    // pemain menyerang
  CHECK_EQ(recorder.events[1].detail, std::string("Goblin"));  // musuh membalas
  CHECK_EQ(recorder.events[1].value, 2);
  CHECK_EQ(c.playerHp(), Player::kMaxHp - 2);
  CHECK_EQ(c.playerPos(), here);  // menyerang tidak berpindah
  CHECK(logContains(c, kGoblinTaunt));  // taunt masuk log

  // Serangan kedua: Goblin mati sebelum sempat membalas.
  recorder.events.clear();
  c.movePlayer(toward);
  CHECK_EQ(recorder.count(EventType::EnemyDied), 1);
  CHECK_EQ(c.playerHp(), Player::kMaxHp - 2);
  CHECK_EQ(c.summary().kills, 1);
  CHECK_EQ(c.summary().score, 25);  // 0 lantai + 1 kill * 25 + 0 langkah
  CHECK(c.dungeon().entityAt(goblinPos) == nullptr);
}

void testDeathWithActiveStatusesEndsCleanly() {
  GameController c;
  Recorder recorder;
  c.addObserver(&recorder);
  c.startRun("Budi", "11", Difficulty::Normal);
  quiet(c);

  Player& me = playerOf(c);
  me.addStatus(std::make_unique<Overconfident>(1000));
  me.addStatus(std::make_unique<Hiccups>(50));
  me.takeDamage(Player::kMaxHp - 1);  // sisa 1 hp

  Dungeon& d = mutableDungeon(c);
  Dir toward = Dir::None;
  CHECK(freeNeighborDir(d, c.playerPos(), toward));
  d.add(std::make_unique<Goblin>(step(c.playerPos(), toward)));

  recorder.events.clear();
  c.movePlayer(Dir::None);  // diam; Goblin menyerang

  CHECK(c.isOver());
  CHECK_EQ(c.playerHp(), 0);
  CHECK_EQ(recorder.count(EventType::PlayerDied), 1);
  CHECK(recorder.events.back().type == EventType::PlayerDied);
  CHECK_EQ(recorder.events.back().detail, std::string("Goblin"));
  CHECK(logContains(c, kGoblinTaunt));

  // Semua query tetap aman setelah mati, status tidak membuat crash.
  CHECK_EQ(c.statusNames().size(), static_cast<std::size_t>(2));
  CHECK_EQ(c.summary().kills, 0);

  // Aksi setelah game over melempar.
  CHECK_THROWS(c.movePlayer(Dir::Up), InvalidMoveException);
  CHECK_THROWS(c.useItem(0), InvalidMoveException);
  CHECK_EQ(recorder.count(EventType::PlayerDied), 1);  // tidak terpicu lagi

  // Run baru mereset semuanya.
  c.startRun("Budi", "12", Difficulty::Normal);
  CHECK(!c.isOver());
  CHECK(c.statusNames().empty());
  CHECK_EQ(c.depth(), 1);
  CHECK_EQ(c.summary().kills, 0);
  CHECK_EQ(c.playerHp(), Player::kMaxHp);
  CHECK(!logContains(c, kGoblinTaunt));
}

void testDescendingStairsTriggersFloorChanged() {
  GameController c;
  Recorder recorder;
  c.addObserver(&recorder);
  c.startRun("Budi", "3", Difficulty::Normal);
  quiet(c);

  Player& me = playerOf(c);
  me.takeDamage(5);
  me.addStatus(std::make_unique<Overconfident>(10));

  Dir toward = Dir::None;
  CHECK(placeBesideStairs(c, toward));

  recorder.events.clear();
  c.movePlayer(toward);

  CHECK(recorder.events.size() >= 2);
  CHECK(recorder.events[0].type == EventType::Moved);
  CHECK(recorder.events[1].type == EventType::FloorChanged);
  CHECK_EQ(recorder.events[1].value, 2);
  CHECK_EQ(recorder.count(EventType::Attacked), 0);  // tanpa fase musuh

  // Lantai baru, pemain dan inventarisnya ikut turun.
  CHECK_EQ(c.depth(), 2);
  CHECK_EQ(c.playerPos(), c.dungeon().start());
  CHECK_EQ(c.playerHp(), Player::kMaxHp - 5);
  CHECK_EQ(c.inventoryNames(),
           (std::vector<std::string>{"Ramuan", "Gulungan Oleng"}));
  CHECK_EQ(c.statusNames(),
           (std::vector<std::string>{"Terlalu Percaya Diri"}));
  CHECK_EQ(countEnemies(c.dungeon()), 2 + 2 + 1);  // 2 + lantai + Normal
  CHECK_EQ(manhattan(petRockPos(c.dungeon()), c.dungeon().start()), 1);
  CHECK_EQ(c.summary().steps, 1);
  CHECK_EQ(c.summary().score, 101);  // 1 lantai * 100 + 0 kill + 1 langkah
}

std::vector<Chest*> chestsOf(Dungeon& d) {
  std::vector<Chest*> out;
  for (auto& e : d.entities()) {
    if (Chest* chest = dynamic_cast<Chest*>(e.get())) out.push_back(chest);
  }
  return out;
}

void testChestsPlacedInLootRoom() {
  int floorsWithLoot = 0;
  int floorsWithout = 0;
  std::set<std::size_t> chestCounts;
  for (unsigned seed = 1; seed <= 300; ++seed) {
    GameController c;
    c.startRun("Tester", std::to_string(seed), Difficulty::Normal);
    Dungeon& d = mutableDungeon(c);
    const auto chests = chestsOf(d);
    if (!d.lootCenter().has_value()) {
      ++floorsWithout;
      CHECK(chests.empty());
      continue;
    }
    ++floorsWithLoot;
    chestCounts.insert(chests.size());
    CHECK(chests.size() >= 1 && chests.size() <= 3);
    std::set<std::pair<int, int>> seen;
    for (const Chest* chest : chests) {
      CHECK(d.tile(chest->pos()).isSafe());
      CHECK(chest->pos() != *d.lootCenter());
      CHECK(seen.insert({chest->pos().x, chest->pos().y}).second);
      CHECK(!chest->loot().empty());
      CHECK_EQ(chest->loot()[0]->name(), std::string("Ramuan"));
      CHECK(chest->loot().size() <= 2);
    }
  }
  CHECK(floorsWithLoot > 0);
  CHECK(floorsWithout > 0);
  CHECK_EQ(chestCounts.size(), static_cast<std::size_t>(3));  // 1, 2, dan 3 muncul
}

void testScrollSometimesAccompaniesPotion() {
  int withScroll = 0;
  int total = 0;
  for (unsigned seed = 1; seed <= 300; ++seed) {
    GameController c;
    c.startRun("Tester", std::to_string(seed), Difficulty::Normal);
    for (Chest* chest : chestsOf(mutableDungeon(c))) {
      ++total;
      if (chest->loot().size() == 2) ++withScroll;
    }
  }
  CHECK(total > 0);
  CHECK(withScroll > 0);
  CHECK(withScroll < total);
}

void testNoEnemySpawnsInSafeZone() {
  for (unsigned seed = 1; seed <= 300; ++seed) {
    GameController c;
    c.startRun("Tester", std::to_string(seed), Difficulty::Sulit);
    const Dungeon& d = c.dungeon();
    for (const auto& e : d.entities()) {
      if (dynamic_cast<const Enemy*>(e.get()) != nullptr) {
        CHECK(!d.tile(e->pos()).isSafe());
      }
    }
  }
}

void testLootRoomAlsoOnLaterFloors() {
  int laterFloorsWithChests = 0;
  for (unsigned seed = 1; seed <= 60; ++seed) {
    GameController c;
    c.startRun("Tester", std::to_string(seed), Difficulty::Mudah);
    quiet(c);
    Dir toward = Dir::None;
    if (!placeBesideStairs(c, toward)) continue;
    c.movePlayer(toward);
    if (c.depth() != 2) continue;
    if (!chestsOf(mutableDungeon(c)).empty()) ++laterFloorsWithChests;
  }
  CHECK(laterFloorsWithChests > 0);
}

// Menaruh peti berisi `loot` di sebelah pemain; mengembalikan arah ke peti.
// Musuh dimatikan dulu supaya skenario bersih.
Dir putChestBesidePlayer(GameController& c, std::vector<std::shared_ptr<Item>> loot) {
  quiet(c);
  Dungeon& d = mutableDungeon(c);
  Dir toward = Dir::None;
  CHECK(freeNeighborDir(d, c.playerPos(), toward));
  d.add(std::make_unique<Chest>(step(c.playerPos(), toward), std::move(loot)));
  return toward;
}

// Menaruh Goblin di petak bebas lain di sebelah pemain (untuk menguji fase musuh).
Goblin* putGoblinBesidePlayer(GameController& c) {
  Dungeon& d = mutableDungeon(c);
  for (Dir dir : kDirs) {
    const Pos p = step(c.playerPos(), dir);
    if (isFree(d, p)) return &static_cast<Goblin&>(d.add(std::make_unique<Goblin>(p)));
  }
  return nullptr;
}

void testOpeningChestGivesItemAndRemovesChest() {
  GameController c;
  Recorder recorder;
  c.addObserver(&recorder);
  c.startRun("Budi", "5", Difficulty::Mudah);
  const Dir toward = putChestBesidePlayer(c, {std::make_shared<Potion>()});
  const Pos start = c.playerPos();
  const std::size_t before = c.inventoryNames().size();

  recorder.events.clear();
  c.movePlayer(toward);

  const std::vector<std::string> names = c.inventoryNames();
  CHECK_EQ(names.size(), before + 1);
  CHECK_EQ(names.back(), std::string("Ramuan"));
  CHECK(chestsOf(mutableDungeon(c)).empty());
  CHECK_EQ(recorder.count(EventType::ChestOpened), 1);
  for (const auto& e : recorder.events) {
    if (e.type == EventType::ChestOpened) {
      CHECK_EQ(e.value, 1);
      CHECK_EQ(e.detail, std::string("Ramuan"));
    }
  }
  CHECK_EQ(c.playerPos(), start);  // membuka peti tidak memindahkan pemain
  CHECK(!c.log().empty());
  CHECK(c.log().back().find("Peti") != std::string::npos);
}

void testChestWithTwoItemsGivesBoth() {
  GameController c;
  c.startRun("Budi", "5", Difficulty::Mudah);
  const Dir toward = putChestBesidePlayer(
      c, {std::make_shared<Potion>(), std::make_shared<Scroll>()});
  c.movePlayer(toward);
  CHECK_EQ(c.inventoryNames(),
           (std::vector<std::string>{"Ramuan", "Gulungan Oleng", "Ramuan",
                                     "Gulungan Oleng"}));
}

void testChestIsNotAKill() {
  GameController c;
  Recorder recorder;
  c.addObserver(&recorder);
  c.startRun("Budi", "5", Difficulty::Mudah);
  const Dir toward = putChestBesidePlayer(c, {std::make_shared<Potion>()});
  const RunSummary before = c.summary();
  recorder.events.clear();
  c.movePlayer(toward);
  const RunSummary after = c.summary();
  CHECK_EQ(after.kills, before.kills);
  CHECK_EQ(after.score, before.score);
  CHECK_EQ(after.steps, before.steps);
  CHECK_EQ(recorder.count(EventType::EnemyDied), 0);
  CHECK_EQ(recorder.count(EventType::Attacked), 0);
  CHECK_EQ(recorder.count(EventType::Moved), 0);
}

void testChestOpeningUsesATurn() {
  GameController c;
  Recorder recorder;
  c.addObserver(&recorder);
  c.startRun("Budi", "5", Difficulty::Mudah);
  const Dir toward = putChestBesidePlayer(c, {std::make_shared<Potion>()});
  CHECK(putGoblinBesidePlayer(c) != nullptr);
  recorder.events.clear();
  c.movePlayer(toward);
  CHECK(recorder.count(EventType::Attacked) >= 1);  // Goblin memukul: fase musuh jalan
  CHECK(c.playerHp() < Player::kMaxHp);
}

void testFullInventoryRejectsChest() {
  GameController c;
  Recorder recorder;
  c.addObserver(&recorder);
  c.startRun("Budi", "5", Difficulty::Mudah);
  const Dir toward = putChestBesidePlayer(c, {std::make_shared<Potion>()});
  Player& me = playerOf(c);
  while (me.inventory().size() < 9) me.inventory().push_back(std::make_shared<Potion>());
  CHECK(putGoblinBesidePlayer(c) != nullptr);
  const int hp = c.playerHp();
  recorder.events.clear();

  c.movePlayer(toward);

  CHECK_EQ(chestsOf(mutableDungeon(c)).size(), static_cast<std::size_t>(1));
  CHECK_EQ(me.inventory().size(), static_cast<std::size_t>(9));
  CHECK_EQ(c.playerHp(), hp);  // giliran tidak terpakai: Goblin tidak bertindak
  CHECK(recorder.events.empty());
  CHECK(c.log().back().find("Tas penuh") != std::string::npos);
}

void testInventoryLimitCountsChestContents() {
  // 8 item + peti 2 item = 10 > 9: ditolak.
  {
    GameController c;
    c.startRun("Budi", "5", Difficulty::Mudah);
    const Dir toward = putChestBesidePlayer(
        c, {std::make_shared<Potion>(), std::make_shared<Scroll>()});
    Player& me = playerOf(c);
    while (me.inventory().size() < 8) me.inventory().push_back(std::make_shared<Potion>());
    c.movePlayer(toward);
    CHECK_EQ(chestsOf(mutableDungeon(c)).size(), static_cast<std::size_t>(1));
    CHECK_EQ(me.inventory().size(), static_cast<std::size_t>(8));
  }
  // 7 item + peti 2 item = 9: diterima.
  {
    GameController c;
    c.startRun("Budi", "5", Difficulty::Mudah);
    const Dir toward = putChestBesidePlayer(
        c, {std::make_shared<Potion>(), std::make_shared<Scroll>()});
    Player& me = playerOf(c);
    while (me.inventory().size() < 7) me.inventory().push_back(std::make_shared<Potion>());
    c.movePlayer(toward);
    CHECK(chestsOf(mutableDungeon(c)).empty());
    CHECK_EQ(me.inventory().size(), static_cast<std::size_t>(9));
  }
}

void testChestDoesNotDisturbObservers() {
  GameController c;
  c.startRun("Budi", "5", Difficulty::Mudah);
  const Dir toward = putChestBesidePlayer(c, {std::make_shared<Potion>()});
  c.movePlayer(toward);
  c.movePlayer(Dir::None);  // masih bisa melanjutkan permainan
  CHECK(!c.isOver());
  CHECK_EQ(c.summary().kills, 0);
}

void testRandomPlayWithChestsAndHiccups() {
  for (unsigned seed = 1; seed <= 40; ++seed) {
    GameController c;
    c.startRun("Budi", std::to_string(seed), Difficulty::Mudah);
    quiet(c);
    playerOf(c).addStatus(std::make_unique<Hiccups>(60));
    Dungeon& d = mutableDungeon(c);
    Dir toward = Dir::None;
    if (freeNeighborDir(d, c.playerPos(), toward)) {
      d.add(std::make_unique<Chest>(step(c.playerPos(), toward),
                                    std::vector<std::shared_ptr<Item>>{std::make_shared<Potion>()}));
    }
    for (int i = 0; i < 50 && !c.isOver(); ++i) c.movePlayer(kDirs[(seed + i) % 4]);
    CHECK(c.dungeon().inBounds(c.playerPos()));
  }
}

// Simulasi acak: musuh tidak pernah berdiri di zona aman dan tas tidak pernah
// melebihi 9 item, sambil pemain berjalan acak dan menabrak peti.
void testRandomPlayKeepsSafeZoneAndInventoryInvariants() {
  for (unsigned seed = 1; seed <= 120; ++seed) {
    GameController c;
    c.startRun("Budi", std::to_string(seed), Difficulty::Sulit);
    Rng dice(seed);
    for (int i = 0; i < 150 && !c.isOver(); ++i) {
      try {
        c.movePlayer(kDirs[static_cast<std::size_t>(dice() % 4)]);
      } catch (const GameException&) {
      }
      const Dungeon& d = c.dungeon();
      for (const auto& e : d.entities()) {
        if (e->isAlive() && dynamic_cast<const Enemy*>(e.get()) != nullptr) {
          CHECK(!d.tile(e->pos()).isSafe());
        }
      }
      CHECK(c.inventoryNames().size() <= 9);
    }
  }
}

// Turun lantai tidak memicu fase musuh: musuh baru belum sempat bergerak,
// jadi semuanya masih berjarak >= 5 dari titik awal. Bila fase musuh ikut
// berjalan, musuh yang tepat berjarak 5 akan maju satu langkah (jarak 4).
void testFloorChangeSkipsEnemyPhase() {
  int checkedEnemies = 0;
  for (std::uint32_t seed = 1; seed <= 40; ++seed) {
    GameController c;
    c.startRun("Budi", std::to_string(seed), Difficulty::Sulit);
    quiet(c);
    Dir toward = Dir::None;
    if (!placeBesideStairs(c, toward)) continue;
    c.movePlayer(toward);
    CHECK_EQ(c.depth(), 2);

    const Dungeon& d = c.dungeon();
    for (const auto& e : d.entities()) {
      if (dynamic_cast<const Enemy*>(e.get()) == nullptr) continue;
      ++checkedEnemies;
      CHECK(manhattan(e->pos(), d.start()) >= 5);
    }
  }
  CHECK(checkedEnemies > 100);
}

void testUseItem() {
  GameController c;
  Recorder recorder;
  c.addObserver(&recorder);
  c.startRun("Budi", "21", Difficulty::Mudah);
  quiet(c);

  // Indeks tidak valid.
  std::string message;
  try {
    c.useItem(5);
  } catch (const InvalidMoveException& e) {
    message = e.what();
  }
  CHECK_EQ(message, std::string("Item tidak ditemukan."));

  // Ramuan: hp pulih, item hilang, tidak ada status baru.
  playerOf(c).takeDamage(10);
  recorder.events.clear();
  c.useItem(0);
  CHECK_EQ(c.playerHp(), Player::kMaxHp - 10 + 8);
  CHECK_EQ(c.inventoryNames(), (std::vector<std::string>{"Gulungan Oleng"}));
  CHECK_EQ(recorder.count(EventType::StatusApplied), 0);

  // Gulungan: status Cegukan bertambah dan event dipancarkan.
  c.useItem(0);
  CHECK_EQ(recorder.count(EventType::StatusApplied), 1);
  CHECK_EQ(recorder.events.back().detail, std::string("Cegukan"));
  CHECK_EQ(c.statusNames(), (std::vector<std::string>{"Cegukan"}));
  CHECK(c.inventoryNames().empty());

  // Inventaris kosong.
  CHECK_THROWS(c.useItem(0), InvalidMoveException);
}

void testLogIsCappedAt100Lines() {
  GameController c;
  c.startRun("Budi", "4", Difficulty::Mudah);
  quiet(c);
  for (int i = 0; i < 150; ++i) c.movePlayer(Dir::None);
  CHECK_EQ(c.log().size(), static_cast<std::size_t>(100));
}

// --- Determinisme ---------------------------------------------------------

struct Trace {
  std::deque<std::string> log;
  std::string fortune;
  int score = 0;
  int depth = 0;
  int kills = 0;
  int steps = 0;
  int hp = 0;
  bool over = false;
  Pos pos;
  Pos start;
  Pos stairs;
  std::vector<EventType> types;
  std::vector<std::string> details;
};

Trace play(const std::string& seedText, Difficulty difficulty) {
  GameController c;
  Recorder recorder;
  c.addObserver(&recorder);
  c.startRun("Budi", seedText, difficulty);

  const Dir script[12] = {Dir::Right, Dir::Right, Dir::Down,  Dir::Down,
                          Dir::Left,  Dir::None,  Dir::Up,    Dir::Right,
                          Dir::Down,  Dir::Down,  Dir::Right, Dir::Up};
  for (int i = 0; i < 300 && !c.isOver(); ++i) {
    if (i % 41 == 40 && !c.inventoryNames().empty()) {
      c.useItem(0);
    } else {
      c.movePlayer(script[(i * 5 + i / 7) % 12]);
    }
  }

  Trace t;
  t.log = c.log();
  t.fortune = c.fortune();
  const RunSummary s = c.summary();
  t.score = s.score;
  t.depth = s.depth;
  t.kills = s.kills;
  t.steps = s.steps;
  t.hp = c.playerHp();
  t.over = c.isOver();
  t.pos = c.playerPos();
  t.start = c.dungeon().start();
  t.stairs = c.dungeon().stairs();
  for (const auto& e : recorder.events) {
    t.types.push_back(e.type);
    t.details.push_back(e.detail);
  }
  return t;
}

void expectSameTrace(const Trace& a, const Trace& b) {
  CHECK(a.log == b.log);
  CHECK_EQ(a.fortune, b.fortune);
  CHECK_EQ(a.score, b.score);
  CHECK_EQ(a.depth, b.depth);
  CHECK_EQ(a.kills, b.kills);
  CHECK_EQ(a.steps, b.steps);
  CHECK_EQ(a.hp, b.hp);
  CHECK_EQ(a.over, b.over);
  CHECK_EQ(a.pos, b.pos);
  CHECK_EQ(a.start, b.start);
  CHECK_EQ(a.stairs, b.stairs);
  CHECK(a.types == b.types);
  CHECK(a.details == b.details);
}

void testSameSeedGivesIdenticalRun() {
  for (std::uint32_t seed : {1u, 2u, 3u, 42u}) {
    const Trace a = play(std::to_string(seed), Difficulty::Normal);
    const Trace b = play(std::to_string(seed), Difficulty::Normal);
    expectSameTrace(a, b);
  }

  // "7" dan "000000007" adalah seed yang sama.
  expectSameTrace(play("7", Difficulty::Sulit), play("000000007", Difficulty::Sulit));

  // Seed berbeda umumnya memberi peta berbeda.
  std::set<std::pair<int, int>> layouts;
  for (std::uint32_t seed = 1; seed <= 10; ++seed) {
    const Trace t = play(std::to_string(seed), Difficulty::Mudah);
    layouts.insert({t.start.x * 100 + t.start.y, t.stairs.x * 100 + t.stairs.y});
  }
  CHECK(layouts.size() >= 2);
}

// --- Sifat yang harus selalu benar saat bermain acak -----------------------

bool stateIsConsistent(const GameController& c) {
  const Dungeon& d = c.dungeon();
  if (c.playerHp() < 0 || c.playerHp() > Player::kMaxHp) return false;
  if (!d.inBounds(c.playerPos()) || !d.tile(c.playerPos()).isWalkable()) {
    return false;
  }
  if (c.log().size() > 100) return false;

  const auto& all = d.entities();
  for (std::size_t i = 0; i < all.size(); ++i) {
    if (!all[i]->isAlive()) continue;
    for (std::size_t j = i + 1; j < all.size(); ++j) {
      if (all[j]->isAlive() && all[i]->pos() == all[j]->pos()) return false;
    }
  }

  const RunSummary s = c.summary();
  return s.score == (s.depth - 1) * 100 + s.kills * 25 + s.steps;
}

void testRandomPlayKeepsInvariants() {
  for (std::uint32_t seed = 1; seed <= 12; ++seed) {
    GameController c;
    c.startRun("Budi", std::to_string(seed), Difficulty::Normal);
    Rng picker(seed * 7919u);

    bool noUnexpectedThrow = true;
    bool consistent = true;
    for (int i = 0; i < 250 && !c.isOver(); ++i) {
      const int roll = std::uniform_int_distribution<int>(0, 5)(picker);
      try {
        if (roll == 5 && !c.inventoryNames().empty()) {
          c.useItem(0);
        } else if (roll == 4) {
          c.movePlayer(Dir::None);
        } else {
          c.movePlayer(kDirs[roll % 4]);
        }
      } catch (const GameException&) {
        noUnexpectedThrow = false;
      }
      if (!stateIsConsistent(c)) consistent = false;
    }
    CHECK(noUnexpectedThrow);
    CHECK(consistent);
  }
}

}  // namespace

int main() {
  testNameValidation();
  testSeedParsing();
  testFortuneTeller();
  testActionsBeforeStartThrow();
  testStartRunRejectsBadInput();
  testRunStartedEventAndPlayerName();
  testFortuneComesRightAfterSeeding();
  testInitialFloor();
  testWallBumpDoesNotUseTurn();
  testHeldBackByStatus();
  testNeverLeavesMap();
  testAttackKillsEnemyAndTauntIsLogged();
  testDeathWithActiveStatusesEndsCleanly();
  testDescendingStairsTriggersFloorChanged();
  testFloorChangeSkipsEnemyPhase();
  testChestsPlacedInLootRoom();
  testScrollSometimesAccompaniesPotion();
  testNoEnemySpawnsInSafeZone();
  testLootRoomAlsoOnLaterFloors();
  testOpeningChestGivesItemAndRemovesChest();
  testChestWithTwoItemsGivesBoth();
  testChestIsNotAKill();
  testChestOpeningUsesATurn();
  testFullInventoryRejectsChest();
  testInventoryLimitCountsChestContents();
  testChestDoesNotDisturbObservers();
  testRandomPlayWithChestsAndHiccups();
  testRandomPlayKeepsSafeZoneAndInventoryInvariants();
  testUseItem();
  testLogIsCappedAt100Lines();
  testSameSeedGivesIdenticalRun();
  testRandomPlayKeepsInvariants();
  return check_summary();
}
