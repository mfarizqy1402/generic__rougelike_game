// Tes Entity, Movement, musuh (Goblin/Slime/Ghost), Player, PetRock,
// dan query entity pada Dungeon.
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "check.hpp"
#include "core/Chest.hpp"
#include "core/Dungeon.hpp"
#include "core/Enemy.hpp"
#include "core/Entity.hpp"
#include "core/Ghost.hpp"
#include "core/Goblin.hpp"
#include "core/Movement.hpp"
#include "core/PetRock.hpp"
#include "core/Potion.hpp"
#include "core/Player.hpp"
#include "core/Rng.hpp"
#include "core/Scroll.hpp"
#include "core/Slime.hpp"
#include "core/Tile.hpp"
#include "core/Types.hpp"

using namespace rogue;

namespace {

// Entity minimal untuk menguji perilaku bawaan kelas dasar abstrak.
class Dummy : public Entity {
 public:
  Dummy(int hp, Pos pos) : Entity("Dummy", hp, pos) {}
  void act(Dungeon&, Rng&) override {}
};

// Ruang terbuka 12x10: bagian dalam (1..10, 1..8) adalah lantai.
Dungeon openRoom() {
  Dungeon d(12, 10);
  for (int y = 1; y <= 8; ++y) {
    for (int x = 1; x <= 10; ++x) d.tile(Pos{x, y}).setType(Floor);
  }
  return d;
}

// Ruang terbuka dengan tembok vertikal di x = 5.
Dungeon walledRoom() {
  Dungeon d = openRoom();
  for (int y = 1; y <= 8; ++y) d.tile(Pos{5, y}).setType(Wall);
  return d;
}

template <class T>
T& put(Dungeon& d, Pos p) {
  return static_cast<T&>(d.add(std::make_unique<T>(p)));
}

Player& putPlayer(Dungeon& d, Pos p) {
  return static_cast<Player&>(d.add(std::make_unique<Player>("Tes", p)));
}

void testEntityBase() {
  Dummy dummy(5, Pos{1, 1});
  CHECK_EQ(dummy.name(), std::string("Dummy"));
  CHECK_EQ(dummy.hp(), 5);
  CHECK(dummy.isAlive());
  CHECK_EQ(dummy.pos(), (Pos{1, 1}));

  dummy.setPos(Pos{3, 4});
  CHECK_EQ(dummy.pos(), (Pos{3, 4}));

  // taunt() bawaan kosong.
  CHECK_EQ(dummy.taunt(), std::string(""));

  dummy.takeDamage(2);
  CHECK_EQ(dummy.hp(), 3);
  dummy.takeDamage(-4);  // damage negatif tidak menyembuhkan
  CHECK_EQ(dummy.hp(), 3);
  dummy.takeDamage(10);  // hp tidak di bawah 0
  CHECK_EQ(dummy.hp(), 0);
  CHECK(!dummy.isAlive());
}

void testTauntsThroughBasePointer() {
  std::vector<std::unique_ptr<Entity>> all;
  all.push_back(std::make_unique<Goblin>(Pos{1, 1}));
  all.push_back(std::make_unique<Slime>(Pos{2, 1}));
  all.push_back(std::make_unique<Ghost>(Pos{3, 1}));

  CHECK_EQ(all[0]->taunt(), std::string("Goblin: Hehehe, dompetmu kelihatan berat!"));
  CHECK_EQ(all[1]->taunt(), std::string("Slime: Plop. Plop. Blub."));
  CHECK_EQ(all[2]->taunt(),
           std::string("Ghost: Uuuuh... aku lupa cara menakut-nakuti."));
  CHECK(all[0]->taunt() != all[1]->taunt());
  CHECK(all[1]->taunt() != all[2]->taunt());
  CHECK(all[0]->taunt() != all[2]->taunt());

  PetRock rock(Pos{4, 1});
  CHECK_EQ(rock.taunt(), std::string("Pet Rock: ..."));
}

void testEnemyStats() {
  Goblin goblin(Pos{1, 1});
  CHECK_EQ(goblin.name(), std::string("Goblin"));
  CHECK_EQ(goblin.hp(), 6);
  CHECK_EQ(goblin.damage(), 2);
  CHECK(!goblin.attackedLastTurn());

  Slime slime(Pos{1, 1});
  CHECK_EQ(slime.name(), std::string("Slime"));
  CHECK_EQ(slime.hp(), 10);
  CHECK_EQ(slime.damage(), 1);

  Ghost ghost(Pos{1, 1});
  CHECK_EQ(ghost.name(), std::string("Ghost"));
  CHECK_EQ(ghost.hp(), 4);
  CHECK_EQ(ghost.damage(), 3);

  PetRock rock(Pos{1, 1});
  CHECK_EQ(rock.name(), std::string("Pet Rock"));
  CHECK_EQ(rock.hp(), 1);
}

void testPlayer() {
  Player player("Budi", Pos{2, 2});
  CHECK_EQ(player.name(), std::string("Budi"));
  CHECK_EQ(Player::kMaxHp, 20);
  CHECK_EQ(player.hp(), Player::kMaxHp);
  CHECK_EQ(player.depth(), 1);
  CHECK(player.inventory().empty());

  player.setDepth(3);
  CHECK_EQ(player.depth(), 3);

  // act() pemain kosong: tidak mengubah apa pun.
  Dungeon d = openRoom();
  Rng rng(1);
  player.act(d, rng);
  CHECK_EQ(player.pos(), (Pos{2, 2}));
  CHECK_EQ(player.hp(), Player::kMaxHp);

  player.takeDamage(12);
  CHECK_EQ(player.hp(), 8);
  player.heal(5);
  CHECK_EQ(player.hp(), 13);
  player.heal(100);  // dibatasi kMaxHp
  CHECK_EQ(player.hp(), Player::kMaxHp);
}

void testGoblinChases() {
  Dungeon d = openRoom();
  Player& player = putPlayer(d, Pos{2, 5});
  Goblin& goblin = put<Goblin>(d, Pos{8, 5});
  Rng rng(1);

  goblin.act(d, rng);
  CHECK_EQ(goblin.pos(), (Pos{7, 5}));
  CHECK(!goblin.attackedLastTurn());
  CHECK_EQ(player.hp(), Player::kMaxHp);
}

void testChaseAxisOrder() {
  Rng rng(1);

  // Selisih x lebih besar: bergerak di sumbu x dulu.
  {
    Dungeon d = openRoom();
    putPlayer(d, Pos{2, 5});
    Goblin& g = put<Goblin>(d, Pos{8, 2});
    g.act(d, rng);
    CHECK_EQ(g.pos(), (Pos{7, 2}));
  }
  // Selisih y lebih besar: bergerak di sumbu y dulu.
  {
    Dungeon d = openRoom();
    putPlayer(d, Pos{5, 6});
    Goblin& g = put<Goblin>(d, Pos{4, 1});
    g.act(d, rng);
    CHECK_EQ(g.pos(), (Pos{4, 2}));
  }
  // Selisih sama besar: sumbu x dulu.
  {
    Dungeon d = openRoom();
    putPlayer(d, Pos{3, 3});
    Goblin& g = put<Goblin>(d, Pos{6, 6});
    g.act(d, rng);
    CHECK_EQ(g.pos(), (Pos{5, 6}));
  }
}

void testGoblinAttacks() {
  Dungeon d = openRoom();
  Player& player = putPlayer(d, Pos{2, 5});
  Goblin& goblin = put<Goblin>(d, Pos{3, 5});
  Rng rng(1);

  goblin.act(d, rng);
  CHECK_EQ(player.hp(), Player::kMaxHp - 2);
  CHECK(goblin.attackedLastTurn());
  CHECK_EQ(goblin.pos(), (Pos{3, 5}));  // menyerang, tidak bergerak

  // Bersebelahan secara diagonal (jarak Manhattan 2) belum menyerang.
  Dungeon d2 = openRoom();
  Player& player2 = putPlayer(d2, Pos{2, 5});
  Goblin& diagonal = put<Goblin>(d2, Pos{3, 6});
  diagonal.act(d2, rng);
  CHECK_EQ(player2.hp(), Player::kMaxHp);
  CHECK(!diagonal.attackedLastTurn());
}

void testEnemyWithoutPlayerDoesNothing() {
  Dungeon d = openRoom();
  Goblin& goblin = put<Goblin>(d, Pos{4, 4});
  Rng rng(1);
  goblin.act(d, rng);
  CHECK_EQ(goblin.pos(), (Pos{4, 4}));
  CHECK(!goblin.attackedLastTurn());
}

void testSlimeActsEveryOtherTurn() {
  Rng rng(1);

  // Bergerak hanya pada pemanggilan act ke-2, 4, ...
  {
    Dungeon d = openRoom();
    putPlayer(d, Pos{2, 5});
    Slime& slime = put<Slime>(d, Pos{8, 5});
    slime.act(d, rng);
    CHECK_EQ(slime.pos(), (Pos{8, 5}));
    slime.act(d, rng);
    CHECK_EQ(slime.pos(), (Pos{7, 5}));
    slime.act(d, rng);
    CHECK_EQ(slime.pos(), (Pos{7, 5}));
    slime.act(d, rng);
    CHECK_EQ(slime.pos(), (Pos{6, 5}));
  }
  // Menyerang hanya pada giliran genap; giliran ganjil bendera serangan padam.
  {
    Dungeon d = openRoom();
    Player& player = putPlayer(d, Pos{2, 5});
    Slime& slime = put<Slime>(d, Pos{3, 5});
    slime.act(d, rng);
    CHECK_EQ(player.hp(), Player::kMaxHp);
    CHECK(!slime.attackedLastTurn());
    slime.act(d, rng);
    CHECK_EQ(player.hp(), Player::kMaxHp - 1);
    CHECK(slime.attackedLastTurn());
    slime.act(d, rng);
    CHECK_EQ(player.hp(), Player::kMaxHp - 1);
    CHECK(!slime.attackedLastTurn());
  }
}

void testGhostPassesWallsGoblinDoesNot() {
  Rng rng(1);

  // Goblin berhenti di depan tembok.
  {
    Dungeon d = walledRoom();
    putPlayer(d, Pos{2, 5});
    Goblin& goblin = put<Goblin>(d, Pos{7, 5});
    goblin.act(d, rng);
    CHECK_EQ(goblin.pos(), (Pos{6, 5}));
    goblin.act(d, rng);
    goblin.act(d, rng);
    CHECK_EQ(goblin.pos(), (Pos{6, 5}));
  }
  // Ghost menembus tembok lalu menyerang setelah menempel.
  {
    Dungeon d = walledRoom();
    Player& player = putPlayer(d, Pos{2, 5});
    Ghost& ghost = put<Ghost>(d, Pos{7, 5});
    ghost.act(d, rng);
    CHECK_EQ(ghost.pos(), (Pos{6, 5}));
    ghost.act(d, rng);
    CHECK_EQ(ghost.pos(), (Pos{5, 5}));
    CHECK_EQ(d.tile(ghost.pos()).type(), Wall);  // berdiri di dalam tembok
    ghost.act(d, rng);
    ghost.act(d, rng);
    CHECK_EQ(ghost.pos(), (Pos{3, 5}));
    ghost.act(d, rng);
    CHECK_EQ(player.hp(), Player::kMaxHp - 3);
    CHECK(ghost.attackedLastTurn());
  }
}

void testMoveTowardRules() {
  // Tidak bisa melangkah ke petak yang ditempati entity hidup.
  {
    Dungeon d = openRoom();
    Goblin& blocker = put<Goblin>(d, Pos{6, 5});
    Goblin& mover = put<Goblin>(d, Pos{7, 5});
    CHECK(!moveToward(d, mover, Pos{2, 5}, false));
    CHECK_EQ(mover.pos(), (Pos{7, 5}));

    // Entity yang sudah mati tidak menghalangi.
    blocker.takeDamage(100);
    CHECK(moveToward(d, mover, Pos{2, 5}, false));
    CHECK_EQ(mover.pos(), (Pos{6, 5}));
  }
  // Sumbu utama terhalang: coba sumbu lain.
  {
    Dungeon d = openRoom();
    put<Goblin>(d, Pos{6, 5});
    Goblin& mover = put<Goblin>(d, Pos{7, 5});
    CHECK(moveToward(d, mover, Pos{2, 6}, false));
    CHECK_EQ(mover.pos(), (Pos{7, 6}));
  }
  // Sudah di tujuan: tidak ada langkah.
  {
    Dungeon d = openRoom();
    Goblin& mover = put<Goblin>(d, Pos{4, 4});
    CHECK(!moveToward(d, mover, Pos{4, 4}, false));
  }
  // Tujuan di luar peta ditolak, bahkan untuk yang tembus dinding.
  {
    Dungeon d = openRoom();
    Ghost& ghost = put<Ghost>(d, Pos{0, 0});
    CHECK(!moveToward(d, ghost, Pos{-5, 0}, true));
    CHECK_EQ(ghost.pos(), (Pos{0, 0}));
  }
}

void testPetRockFollowsAndIsInvincible() {
  Dungeon d = openRoom();
  Player& player = putPlayer(d, Pos{2, 5});
  PetRock& rock = put<PetRock>(d, Pos{8, 5});
  Rng rng(1);

  for (int i = 0; i < 20; ++i) rock.act(d, rng);
  CHECK_EQ(rock.pos(), (Pos{3, 5}));  // berhenti saat menempel
  CHECK_EQ(player.hp(), Player::kMaxHp);

  rock.takeDamage(5);
  CHECK_EQ(rock.hp(), 1);
  CHECK(rock.isAlive());
  rock.takeDamage(1000);
  CHECK(rock.isAlive());
}

void testDungeonEntityQueries() {
  Dungeon d = openRoom();
  CHECK(d.entities().empty());
  CHECK(d.player() == nullptr);
  CHECK(d.entityAt(Pos{3, 3}) == nullptr);

  // add() mengembalikan referensi ke objek yang sama.
  auto goblinPtr = std::make_unique<Goblin>(Pos{3, 3});
  Goblin* raw = goblinPtr.get();
  Entity& added = d.add(std::move(goblinPtr));
  CHECK(&added == raw);
  CHECK_EQ(d.entities().size(), static_cast<std::size_t>(1));
  CHECK(d.entityAt(Pos{3, 3}) == raw);
  CHECK(d.entityAt(Pos{4, 3}) == nullptr);
  CHECK(d.player() == nullptr);  // Goblin bukan Player

  Player& player = putPlayer(d, Pos{5, 5});
  CHECK(d.player() == &player);
  CHECK_EQ(d.entities().size(), static_cast<std::size_t>(2));

  // Versi const.
  const Dungeon& view = d;
  CHECK(view.entityAt(Pos{3, 3}) == raw);
  CHECK(view.player() == &player);
  CHECK_EQ(view.entities().size(), static_cast<std::size_t>(2));

  // entityAt hanya mengembalikan entity hidup; entity mati tetap di daftar.
  raw->takeDamage(100);
  CHECK(d.entityAt(Pos{3, 3}) == nullptr);
  CHECK_EQ(d.entities().size(), static_cast<std::size_t>(2));
}

void testEnemiesAvoidSafeTiles() {
  // Goblin tidak melangkah ke petak safe; moveToward tanpa avoidSafe tetap masuk.
  {
    Dungeon d = openRoom();
    d.tile(Pos{5, 5}).setSafe(true);
    d.tile(Pos{6, 5}).setSafe(true);
    putPlayer(d, Pos{8, 5});
    Goblin& goblin = put<Goblin>(d, Pos{4, 5});
    Rng rng(1);
    goblin.act(d, rng);
    CHECK_EQ(goblin.pos(), (Pos{4, 5}));

    Dummy probe(5, Pos{4, 5});
    CHECK(!moveToward(d, probe, Pos{8, 5}, false, true));
    CHECK_EQ(probe.pos(), (Pos{4, 5}));
    CHECK(moveToward(d, probe, Pos{8, 5}, false, false));
    CHECK_EQ(probe.pos(), (Pos{5, 5}));
  }
  // Sumbu lain dicoba bila sumbu utama safe.
  {
    Dungeon d = openRoom();
    d.tile(Pos{5, 5}).setSafe(true);
    Dummy probe(5, Pos{4, 5});
    CHECK(moveToward(d, probe, Pos{8, 6}, false, true));
    CHECK_EQ(probe.pos(), (Pos{4, 6}));
  }
  // Hantu (tembus tembok) pun tidak masuk petak safe.
  {
    Dungeon d = walledRoom();
    d.tile(Pos{5, 5}).setSafe(true);
    putPlayer(d, Pos{8, 5});
    Ghost& ghost = put<Ghost>(d, Pos{4, 5});
    Rng rng(1);
    ghost.act(d, rng);
    CHECK_EQ(ghost.pos(), (Pos{4, 5}));
  }
  // PetRock boleh mengikuti pemain masuk ke petak safe.
  {
    Dungeon d = openRoom();
    for (int x = 5; x <= 8; ++x) d.tile(Pos{x, 5}).setSafe(true);
    putPlayer(d, Pos{8, 5});
    PetRock& rock = put<PetRock>(d, Pos{4, 5});
    Rng rng(1);
    rock.act(d, rng);
    CHECK_EQ(rock.pos(), (Pos{5, 5}));
  }
}

void testChest() {
  Chest chest(Pos{2, 2}, {std::make_shared<Potion>(), std::make_shared<Scroll>()});
  CHECK_EQ(chest.name(), std::string("Peti"));
  CHECK_EQ(chest.hp(), 1);
  CHECK_EQ(Chest::kHp, 1);
  CHECK(chest.isAlive());
  CHECK_EQ(chest.loot().size(), static_cast<std::size_t>(2));

  // act() tidak melakukan apa pun.
  Dungeon d(12, 10);
  Rng rng(1);
  chest.act(d, rng);
  CHECK(chest.pos() == (Pos{2, 2}));
  CHECK(chest.isAlive());
  CHECK_EQ(chest.loot().size(), static_cast<std::size_t>(2));

  // takeLoot() menyerahkan isi dan mengosongkan peti.
  auto taken = chest.takeLoot();
  CHECK_EQ(taken.size(), static_cast<std::size_t>(2));
  CHECK(chest.loot().empty());

  // Satu serangan cukup untuk memecahkannya; bukan Enemy.
  chest.takeDamage(3);
  CHECK(!chest.isAlive());
  Entity& base = chest;
  CHECK(dynamic_cast<Enemy*>(&base) == nullptr);
}

}  // namespace

int main() {
  testEnemiesAvoidSafeTiles();
  testChest();
  testEntityBase();
  testTauntsThroughBasePointer();
  testEnemyStats();
  testPlayer();
  testGoblinChases();
  testChaseAxisOrder();
  testGoblinAttacks();
  testEnemyWithoutPlayerDoesNothing();
  testSlimeActsEveryOtherTurn();
  testGhostPassesWallsGoblinDoesNot();
  testMoveTowardRules();
  testPetRockFollowsAndIsInvincible();
  testDungeonEntityQueries();
  return check_summary();
}
