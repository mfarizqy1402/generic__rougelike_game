#include "controller/GameController.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <random>
#include <utility>
#include <vector>

#include "controller/InputValidator.hpp"
#include "core/Chest.hpp"
#include "core/Enemy.hpp"
#include "core/Exceptions.hpp"
#include "core/Ghost.hpp"
#include "core/Goblin.hpp"
#include "core/Item.hpp"
#include "core/PetRock.hpp"
#include "core/Player.hpp"
#include "core/Potion.hpp"
#include "core/Scroll.hpp"
#include "core/Slime.hpp"
#include "core/Tile.hpp"

namespace rogue {

namespace {

constexpr int kMapWidth = 40;
constexpr int kMapHeight = 25;
constexpr int kPlayerAttackDamage = 3;
constexpr std::size_t kMaxLogLines = 100;
constexpr int kMinEnemyDistance = 5;
constexpr int kPlacementAttempts = 100;
constexpr std::size_t kMaxInventory = 9;  // hanya tombol 1-9 yang bisa memakai item
constexpr int kChestScrollChance = 30;    // persen
constexpr int kMaxChestsPerRoom = 3;

int randomBetween(Rng& rng, int lo, int hi) {
  return std::uniform_int_distribution<int>(lo, hi)(rng);
}

// Jenis musuh: 0 Goblin, 1 Slime, 2 Ghost.
std::unique_ptr<Entity> makeEnemy(int kind, Pos pos) {
  switch (kind) {
    case 0:
      return std::make_unique<Goblin>(pos);
    case 1:
      return std::make_unique<Slime>(pos);
    default:
      return std::make_unique<Ghost>(pos);
  }
}

}  // namespace

GameController::GameController() = default;
GameController::~GameController() = default;

void GameController::addObserver(IGameObserver* observer) {
  if (observer != nullptr) observers_.push_back(observer);
}

// ---------------------------------------------------------------------------
// Memulai run
// ---------------------------------------------------------------------------

void GameController::startRun(const std::string& name,
                              const std::string& seedText,
                              Difficulty difficulty) {
  // Validasi lebih dulu: bila gagal, state lama tidak tersentuh.
  const std::string cleanName = InputValidator::validateName(name);
  const std::optional<std::uint32_t> seed = InputValidator::parseSeed(seedText);

  if (seed.has_value()) {
    rng_.seed(*seed);
  } else {
    std::random_device device;
    rng_.seed(device());
  }

  active_ = false;
  over_ = false;
  kills_ = 0;
  steps_ = 0;
  difficulty_ = difficulty;
  log_.clear();
  fortune_ = fortuneTeller_.fortune(rng_);  // tepat setelah seed

  auto hero = std::make_unique<Player>(cleanName, Pos{});
  hero->inventory().push_back(std::make_shared<Potion>());
  hero->inventory().push_back(std::make_shared<Scroll>());
  buildFloor(std::move(hero));

  active_ = true;
  pushLog("Selamat datang, " + cleanName + "! Lantai 1 menunggu.");
  pushLog("Ramalan: " + fortune_);
  emit(EventType::RunStarted, cleanName);
}

void GameController::buildFloor(std::unique_ptr<Player> hero) {
  dungeon_ = std::make_unique<Dungeon>(
      generator_.generate(kMapWidth, kMapHeight, rng_));
  const Pos start = dungeon_->start();
  hero->setPos(start);
  dungeon_->add(std::move(hero));
  placePetRock(start);
  placeChests();
  placeEnemies(start);
}

void GameController::placePetRock(Pos start) {
  const Dir order[4] = {Dir::Up, Dir::Down, Dir::Left, Dir::Right};
  for (Dir dir : order) {
    const Pos p = step(start, dir);
    if (dungeon_->inBounds(p) && dungeon_->tile(p).isWalkable() &&
        dungeon_->entityAt(p) == nullptr) {
      dungeon_->add(std::make_unique<PetRock>(p));
      return;
    }
  }
}

void GameController::placeChests() {
  const std::optional<Pos> center = dungeon_->lootCenter();
  if (!center.has_value()) return;

  // Kandidat: petak lantai di zona aman, bukan titik tengah (dilewati koridor).
  std::vector<Pos> candidates;
  for (int y = 0; y < dungeon_->height(); ++y) {
    for (int x = 0; x < dungeon_->width(); ++x) {
      const Pos p{x, y};
      if (!dungeon_->tile(p).isSafe() || dungeon_->tile(p).type() != Floor) continue;
      if (p == *center || dungeon_->entityAt(p) != nullptr) continue;
      candidates.push_back(p);
    }
  }

  const int wanted = randomBetween(rng_, 1, kMaxChestsPerRoom);
  for (int i = 0; i < wanted && !candidates.empty(); ++i) {
    const std::size_t pick = static_cast<std::size_t>(
        randomBetween(rng_, 0, static_cast<int>(candidates.size()) - 1));
    const Pos p = candidates[pick];
    candidates.erase(candidates.begin() + static_cast<std::ptrdiff_t>(pick));

    std::vector<std::shared_ptr<Item>> loot;
    loot.push_back(std::make_shared<Potion>());
    if (randomBetween(rng_, 1, 100) <= kChestScrollChance) {
      loot.push_back(std::make_shared<Scroll>());
    }
    dungeon_->add(std::make_unique<Chest>(p, std::move(loot)));
  }
}

void GameController::placeEnemies(Pos start) {
  const int count = 2 + player().depth() + static_cast<int>(difficulty_);
  for (int i = 0; i < count; ++i) {
    const int kind = randomBetween(rng_, 0, 2);
    for (int attempt = 0; attempt < kPlacementAttempts; ++attempt) {
      const int x = randomBetween(rng_, 1, dungeon_->width() - 2);
      const int y = randomBetween(rng_, 1, dungeon_->height() - 2);
      const Pos p{x, y};
      if (dungeon_->tile(p).type() != Floor) continue;
      if (dungeon_->tile(p).isSafe()) continue;
      if (manhattan(start, p) < kMinEnemyDistance) continue;
      if (dungeon_->entityAt(p) != nullptr) continue;
      dungeon_->add(makeEnemy(kind, p));
      break;
    }
  }
}

// ---------------------------------------------------------------------------
// Aksi pemain
// ---------------------------------------------------------------------------

void GameController::requireRunning() const {
  if (!active_ || !dungeon_) {
    throw InvalidMoveException(
        "Belum ada permainan yang berjalan. Mulai dulu dari menu.");
  }
  if (over_) {
    throw InvalidMoveException("Permainan sudah selesai.");
  }
}

void GameController::movePlayer(Dir requested) {
  requireRunning();
  Player& me = player();

  // Status bekerja: awal giliran, lalu arah diubah berantai.
  for (auto& status : me.statuses()) status->onTurnStart(me);
  Dir dir = requested;
  for (auto& status : me.statuses()) dir = status->modifyMove(dir, rng_);

  // Diam (diminta pemain) atau tertahan (dibatalkan status): memakai giliran.
  if (dir == Dir::None) {
    const bool asked = (requested == Dir::None);
    emit(EventType::Waited, asked ? "diam" : "tertahan");
    pushLog(asked ? "Kamu diam di tempat. Produktif sekali."
                  : "Kamu ingin bergerak, tapi kakimu tidak mau menurut.");
    endTurn(true);
    return;
  }

  // Menabrak tembok atau tepi peta: tidak memakai giliran.
  const Pos dest = step(me.pos(), dir);
  if (!dungeon_->inBounds(dest) || !dungeon_->tile(dest).isWalkable()) {
    emit(EventType::WallBump);
    pushLog("Kamu menabrak tembok. Tembok tidak minta maaf.");
    return;
  }

  // Ada musuh di tujuan: serang.
  Entity* occupant = dungeon_->entityAt(dest);
  if (Enemy* enemy = dynamic_cast<Enemy*>(occupant)) {
    attackEnemy(me, *enemy);
    endTurn(true);
    return;
  }

  // Peti: diserang sampai pecah, isinya masuk ke inventaris.
  if (Chest* chest = dynamic_cast<Chest*>(occupant)) {
    if (me.inventory().size() + chest->loot().size() > kMaxInventory) {
      pushLog("Tas penuh. Peti tidak bisa dibuka.");
      return;  // tidak memakai giliran
    }
    openChest(me, *chest);
    endTurn(true);
    return;
  }

  // Selain musuh dan peti (Pet Rock): tukar tempat. Petak kosong: pindah biasa.
  if (occupant != nullptr) occupant->setPos(me.pos());
  me.setPos(dest);
  ++steps_;
  emit(EventType::Moved);

  if (dungeon_->tile(dest).type() == Stairs) {
    descend();
    endTurn(false);  // lantai baru: tanpa fase musuh
    return;
  }
  endTurn(true);
}

void GameController::attackEnemy(Player& attacker, Enemy& enemy) {
  const std::string enemyName = enemy.name();
  enemy.takeDamage(kPlayerAttackDamage);
  emit(EventType::Attacked, attacker.name(), kPlayerAttackDamage);
  pushLog("Kamu menyerang " + enemyName + " (" +
          std::to_string(kPlayerAttackDamage) + " damage).");

  if (enemy.isAlive()) return;

  const Entity* dead = &enemy;
  auto& all = dungeon_->entities();
  all.erase(std::remove_if(all.begin(), all.end(),
                           [dead](const std::unique_ptr<Entity>& e) {
                             return e.get() == dead;
                           }),
            all.end());
  ++kills_;
  emit(EventType::EnemyDied, enemyName);
  pushLog(enemyName + " tumbang. Dunia sedikit lebih tenang.");
}

void GameController::openChest(Player& me, Chest& chest) {
  chest.takeDamage(kPlayerAttackDamage);  // 1 HP: pecah dalam sekali serang
  const std::vector<std::shared_ptr<Item>> loot = chest.takeLoot();

  std::string names;
  for (const auto& item : loot) {
    if (!names.empty()) names += ", ";
    names += item->name();
    me.inventory().push_back(item);
  }

  const Entity* dead = &chest;
  auto& all = dungeon_->entities();
  all.erase(std::remove_if(all.begin(), all.end(),
                           [dead](const std::unique_ptr<Entity>& e) {
                             return e.get() == dead;
                           }),
            all.end());

  emit(EventType::ChestOpened, names, static_cast<int>(loot.size()));
  pushLog("Kamu membuka Peti. Isinya: " + names + ".");
}

void GameController::useItem(std::size_t index) {
  requireRunning();
  Player& me = player();
  if (index >= me.inventory().size()) {
    throw InvalidMoveException("Item tidak ditemukan.");
  }

  const std::shared_ptr<Item> item = me.inventory()[index];
  const std::size_t statusesBefore = me.statuses().size();
  const std::string message = item->use(me);
  me.inventory().erase(me.inventory().begin() +
                       static_cast<std::ptrdiff_t>(index));
  pushLog(message);

  if (me.statuses().size() > statusesBefore) {
    emit(EventType::StatusApplied, me.statuses().back()->name());
  }
  endTurn(true);
}

// ---------------------------------------------------------------------------
// Akhir giliran
// ---------------------------------------------------------------------------

void GameController::endTurn(bool enemyPhase) {
  if (enemyPhase) {
    runEnemyPhase();
    if (over_) return;  // pemain mati: berhenti, tanpa tick
  }
  tickStatuses();
}

void GameController::runEnemyPhase() {
  Player& me = player();

  // Salinan daftar: urutan tetap dan aman walau isi peta berubah.
  std::vector<Entity*> actors;
  for (const auto& e : dungeon_->entities()) {
    if (e.get() != &me) actors.push_back(e.get());
  }

  for (Entity* actor : actors) {
    if (!actor->isAlive()) continue;
    actor->act(*dungeon_, rng_);  // polimorfik: tiap makhluk berbeda

    const Enemy* enemy = dynamic_cast<const Enemy*>(actor);
    if (enemy == nullptr || !enemy->attackedLastTurn()) continue;

    emit(EventType::Attacked, enemy->name(), enemy->damage());
    const std::string taunt = enemy->taunt();
    if (!taunt.empty()) pushLog(taunt);
    pushLog(enemy->name() + " menyerangmu (-" + std::to_string(enemy->damage()) +
            " HP).");

    if (!me.isAlive()) {
      over_ = true;
      pushLog("Kamu tewas diserang " + enemy->name() +
              ". Setidaknya kamu jadi cerita lucu.");
      emit(EventType::PlayerDied, enemy->name());
      return;
    }
  }
}

void GameController::tickStatuses() {
  auto& statuses = player().statuses();
  for (auto& status : statuses) status->tick();
  statuses.erase(std::remove_if(statuses.begin(), statuses.end(),
                                [](const std::unique_ptr<StatusEffect>& s) {
                                  return s->expired();
                                }),
                 statuses.end());
}

// ---------------------------------------------------------------------------
// Turun lantai
// ---------------------------------------------------------------------------

std::unique_ptr<Player> GameController::extractPlayer() {
  for (auto& e : dungeon_->entities()) {
    if (dynamic_cast<Player*>(e.get()) != nullptr) {
      return std::unique_ptr<Player>(static_cast<Player*>(e.release()));
    }
  }
  throw InvalidMoveException("Pemain tidak ditemukan di peta.");
}

void GameController::descend() {
  std::unique_ptr<Player> hero = extractPlayer();  // hp, inventaris, status ikut
  hero->setDepth(hero->depth() + 1);
  const int newDepth = hero->depth();
  buildFloor(std::move(hero));
  emit(EventType::FloorChanged, "", newDepth);
  pushLog("Kamu turun ke lantai " + std::to_string(newDepth) +
          ". Tangganya tidak ada pegangan.");
}

// ---------------------------------------------------------------------------
// Query
// ---------------------------------------------------------------------------

bool GameController::isOver() const { return over_; }

const Dungeon& GameController::dungeon() const {
  if (!dungeon_) {
    throw InvalidMoveException("Belum ada permainan yang berjalan.");
  }
  return *dungeon_;
}

const std::deque<std::string>& GameController::log() const { return log_; }
const std::string& GameController::fortune() const { return fortune_; }

RunSummary GameController::summary() const {
  RunSummary s;
  s.depth = player().depth();
  s.kills = kills_;
  s.steps = steps_;
  s.score = (s.depth - 1) * 100 + s.kills * 25 + s.steps;
  return s;
}

Pos GameController::playerPos() const { return player().pos(); }
int GameController::playerHp() const { return player().hp(); }
std::string GameController::playerName() const { return player().name(); }
int GameController::depth() const { return player().depth(); }

std::vector<std::string> GameController::inventoryNames() const {
  std::vector<std::string> names;
  for (const auto& item : player().inventory()) names.push_back(item->name());
  return names;
}

std::vector<std::string> GameController::statusNames() const {
  std::vector<std::string> names;
  for (const auto& status : player().statuses()) names.push_back(status->name());
  return names;
}

// ---------------------------------------------------------------------------
// Pembantu privat
// ---------------------------------------------------------------------------

Player& GameController::player() {
  Entity* found = dungeon_ ? dungeon_->player() : nullptr;
  Player* hero = dynamic_cast<Player*>(found);
  if (hero == nullptr) {
    throw InvalidMoveException("Belum ada permainan yang berjalan.");
  }
  return *hero;
}

const Player& GameController::player() const {
  const Entity* found = dungeon_ ? std::as_const(*dungeon_).player() : nullptr;
  const Player* hero = dynamic_cast<const Player*>(found);
  if (hero == nullptr) {
    throw InvalidMoveException("Belum ada permainan yang berjalan.");
  }
  return *hero;
}

void GameController::emit(EventType type, const std::string& detail, int value) {
  const GameEvent event{type, detail, value};
  for (IGameObserver* observer : observers_) observer->onEvent(event);
}

void GameController::pushLog(const std::string& line) {
  log_.push_back(line);
  while (log_.size() > kMaxLogLines) log_.pop_front();
}

}  // namespace rogue
