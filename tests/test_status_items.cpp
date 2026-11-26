// Tes StatusEffect (Hiccups, Overconfident), IUsable/Item (Potion, Scroll).
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "check.hpp"
#include "core/Entity.hpp"
#include "core/Hiccups.hpp"
#include "core/IUsable.hpp"
#include "core/Item.hpp"
#include "core/Overconfident.hpp"
#include "core/Player.hpp"
#include "core/Potion.hpp"
#include "core/Rng.hpp"
#include "core/Scroll.hpp"
#include "core/StatusEffect.hpp"
#include "core/Types.hpp"

using namespace rogue;

namespace {

void testHiccupsChangesAboutOneThird() {
  Hiccups hiccups(5);
  CHECK_EQ(hiccups.name(), std::string("Cegukan"));

  Rng rng(7);
  const Dir inputs[] = {Dir::Up, Dir::Down, Dir::Left, Dir::Right};
  int changed = 0;
  bool producedNone = false;
  for (int i = 0; i < 300; ++i) {
    const Dir in = inputs[i % 4];
    const Dir out = hiccups.modifyMove(in, rng);
    if (out != in) {
      ++changed;
      if (out == Dir::None) producedNone = true;
    }
  }
  CHECK(changed >= 60 && changed <= 140);
  CHECK(!producedNone);
}

void testHiccupsPicksAnyOfThreeOtherDirections() {
  Hiccups hiccups(5);
  Rng rng(11);
  bool seenDown = false;
  bool seenLeft = false;
  bool seenRight = false;
  bool seenUpWhenChanged = false;  // tidak boleh: "diganti" berarti beda arah
  for (int i = 0; i < 600; ++i) {
    const Dir out = hiccups.modifyMove(Dir::Up, rng);
    if (out == Dir::Down) seenDown = true;
    if (out == Dir::Left) seenLeft = true;
    if (out == Dir::Right) seenRight = true;
    if (out == Dir::None) seenUpWhenChanged = true;  // None tidak boleh muncul
  }
  CHECK(seenDown);
  CHECK(seenLeft);
  CHECK(seenRight);
  CHECK(!seenUpWhenChanged);
}

void testHiccupsLeavesNoneAlone() {
  Hiccups hiccups(5);
  Rng rng(3);
  bool allNone = true;
  for (int i = 0; i < 200; ++i) {
    if (hiccups.modifyMove(Dir::None, rng) != Dir::None) allNone = false;
  }
  CHECK(allNone);
}

void testOverconfidentRefusesToGoBack() {
  Overconfident status(5);
  CHECK_EQ(status.name(), std::string("Terlalu Percaya Diri"));
  Rng rng(1);

  // Belum ada gerak sebelumnya: semua arah boleh.
  CHECK(status.modifyMove(Dir::Right, rng) == Dir::Right);
  // Berlawanan dengan gerak terakhir: ditolak.
  CHECK(status.modifyMove(Dir::Left, rng) == Dir::None);
  // Gerak terakhir tetap Right, jadi Left masih ditolak, Right/Up/Down boleh.
  CHECK(status.modifyMove(Dir::Right, rng) == Dir::Right);
  CHECK(status.modifyMove(Dir::Left, rng) == Dir::None);
  CHECK(status.modifyMove(Dir::Up, rng) == Dir::Up);
  // Sekarang gerak terakhir Up, jadi Down ditolak.
  CHECK(status.modifyMove(Dir::Down, rng) == Dir::None);
  // Diam tidak mengubah apa pun.
  CHECK(status.modifyMove(Dir::None, rng) == Dir::None);
  CHECK(status.modifyMove(Dir::Down, rng) == Dir::None);
  CHECK(status.modifyMove(Dir::Left, rng) == Dir::Left);
}

void testStatusExpires() {
  Hiccups hiccups(2);
  CHECK(!hiccups.expired());
  hiccups.tick();
  CHECK(!hiccups.expired());
  hiccups.tick();
  CHECK(hiccups.expired());
  hiccups.tick();  // tidak jatuh ke negatif
  CHECK(hiccups.expired());

  Overconfident zero(0);
  CHECK(zero.expired());
}

void testStatusesOnEntityViaBasePointer() {
  Player player("Tes", Pos{1, 1});
  CHECK(player.statuses().empty());

  player.addStatus(std::make_unique<Hiccups>(3));
  player.addStatus(std::make_unique<Overconfident>(2));
  CHECK_EQ(player.statuses().size(), static_cast<std::size_t>(2));

  StatusEffect& first = *player.statuses()[0];
  StatusEffect& second = *player.statuses()[1];
  CHECK_EQ(first.name(), std::string("Cegukan"));
  CHECK_EQ(second.name(), std::string("Terlalu Percaya Diri"));

  // onTurnStart bawaan tidak mengubah entity.
  first.onTurnStart(player);
  CHECK_EQ(player.hp(), Player::kMaxHp);

  const Player& view = player;
  CHECK_EQ(view.statuses().size(), static_cast<std::size_t>(2));
}

void testPotionHealsCapped() {
  Player player("Tes", Pos{1, 1});
  Potion potion;
  CHECK_EQ(potion.name(), std::string("Ramuan"));

  player.takeDamage(15);  // hp 5
  IUsable& usable = potion;
  const std::string message = usable.use(player);
  CHECK(!message.empty());
  CHECK_EQ(player.hp(), 13);

  potion.use(player);  // 13 + 8 dibatasi kMaxHp
  CHECK_EQ(player.hp(), Player::kMaxHp);
}

void testScrollAddsHiccups() {
  Player player("Tes", Pos{1, 1});
  Scroll scroll;
  CHECK_EQ(scroll.name(), std::string("Gulungan Oleng"));

  const std::string message = scroll.use(player);
  CHECK(!message.empty());
  CHECK_EQ(player.statuses().size(), static_cast<std::size_t>(1));
  CHECK_EQ(player.statuses()[0]->name(), std::string("Cegukan"));
  CHECK(!player.statuses()[0]->expired());

  scroll.use(player);
  CHECK_EQ(player.statuses().size(), static_cast<std::size_t>(2));
}

void testInventoryPolymorphism() {
  Player player("Tes", Pos{1, 1});
  player.inventory().push_back(std::make_shared<Potion>());
  player.inventory().push_back(std::make_shared<Scroll>());
  CHECK_EQ(player.inventory().size(), static_cast<std::size_t>(2));
  CHECK_EQ(player.inventory()[0]->name(), std::string("Ramuan"));
  CHECK_EQ(player.inventory()[1]->name(), std::string("Gulungan Oleng"));

  // Pemanggilan lewat pointer basis memicu use() milik kelas turunan.
  player.takeDamage(10);
  player.inventory()[0]->use(player);
  CHECK_EQ(player.hp(), 18);
  player.inventory()[1]->use(player);
  CHECK_EQ(player.statuses().size(), static_cast<std::size_t>(1));
}

}  // namespace

int main() {
  testHiccupsChangesAboutOneThird();
  testHiccupsPicksAnyOfThreeOtherDirections();
  testHiccupsLeavesNoneAlone();
  testOverconfidentRefusesToGoBack();
  testStatusExpires();
  testStatusesOnEntityViaBasePointer();
  testPotionHealsCapped();
  testScrollAddsHiccups();
  testInventoryPolymorphism();
  return check_summary();
}
