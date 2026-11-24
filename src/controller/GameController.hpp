#pragma once

#include <cstddef>
#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "core/Dungeon.hpp"
#include "core/DungeonGenerator.hpp"
#include "core/FortuneTeller.hpp"
#include "core/GameEvent.hpp"
#include "core/IGameObserver.hpp"
#include "core/Rng.hpp"
#include "core/Types.hpp"

namespace rogue {

class Chest;
class Enemy;
class Player;

enum class Difficulty { Mudah, Normal, Sulit };

// Ringkasan satu run. score = (depth - 1) * 100 + kills * 25 + steps.
struct RunSummary {
  int score = 0;
  int depth = 1;
  int kills = 0;
  int steps = 0;
};

// Satu-satunya pintu masuk logika game bagi lapisan UI. Memiliki satu Rng
// yang disuntikkan ke semua kode core, sehingga seed yang sama menghasilkan
// run yang identik. Murni logika: tidak bergantung pada library tampilan.
class GameController {
 public:
  GameController();
  ~GameController();

  GameController(const GameController&) = delete;
  GameController& operator=(const GameController&) = delete;

  // Pengamat tidak dimiliki controller; pemanggil menjaganya tetap hidup.
  void addObserver(IGameObserver* observer);

  // Validasi dulu (InvalidInputException, state lama tidak berubah), lalu
  // reset semua. Seed kosong berarti seed acak dari std::random_device.
  void startRun(const std::string& name, const std::string& seedText,
                Difficulty difficulty);

  // Aksi pemain. Melempar InvalidMoveException bila belum ada run atau
  // run sudah selesai.
  void movePlayer(Dir dir);
  void useItem(std::size_t index);

  bool isOver() const;
  const Dungeon& dungeon() const;
  const std::deque<std::string>& log() const;
  const std::string& fortune() const;
  RunSummary summary() const;

  Pos playerPos() const;
  int playerHp() const;
  std::string playerName() const;
  int depth() const;
  std::vector<std::string> inventoryNames() const;
  std::vector<std::string> statusNames() const;

 private:
  void requireRunning() const;
  Player& player();
  const Player& player() const;

  void emit(EventType type, const std::string& detail = "", int value = 0);
  void pushLog(const std::string& line);

  void buildFloor(std::unique_ptr<Player> player);
  void placePetRock(Pos start);
  void placeChests();
  void placeEnemies(Pos start);
  std::unique_ptr<Player> extractPlayer();
  void descend();

  void attackEnemy(Player& attacker, Enemy& enemy);
  void openChest(Player& me, Chest& chest);
  void endTurn(bool enemyPhase);
  void runEnemyPhase();
  void tickStatuses();

  Rng rng_;
  FortuneTeller fortuneTeller_;
  DungeonGenerator generator_;
  std::unique_ptr<Dungeon> dungeon_;
  std::vector<IGameObserver*> observers_;
  std::deque<std::string> log_;
  std::string fortune_;
  Difficulty difficulty_ = Difficulty::Normal;
  int kills_ = 0;
  int steps_ = 0;
  bool active_ = false;
  bool over_ = false;
};

}  // namespace rogue
