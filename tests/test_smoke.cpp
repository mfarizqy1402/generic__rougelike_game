// Tes smoke: Types (Pos, Dir, step), Rng, dan hierarki Exceptions.
#include <string>

#include "check.hpp"
#include "core/Exceptions.hpp"
#include "core/Rng.hpp"
#include "core/Types.hpp"

using namespace rogue;

static void throwInvalidInput() { throw InvalidInputException("input buruk"); }
static void throwInvalidMove() { throw InvalidMoveException("gerak buruk"); }
static void throwScoreFile() { throw ScoreFileException("berkas rusak"); }

int main() {
  // --- step(Pos, Dir): sumbu y bertambah ke bawah ---
  const Pos origin{5, 5};
  CHECK_EQ(step(origin, Dir::Up), (Pos{5, 4}));
  CHECK_EQ(step(origin, Dir::Down), (Pos{5, 6}));
  CHECK_EQ(step(origin, Dir::Left), (Pos{4, 5}));
  CHECK_EQ(step(origin, Dir::Right), (Pos{6, 5}));
  CHECK_EQ(step(origin, Dir::None), origin);

  // --- manhattan(Pos, Pos): jarak |dx| + |dy| ---
  CHECK_EQ(manhattan(Pos{0, 0}, Pos{3, 4}), 7);
  CHECK_EQ(manhattan(Pos{3, 4}, Pos{0, 0}), 7);
  CHECK_EQ(manhattan(Pos{2, 2}, Pos{2, 2}), 0);
  CHECK_EQ(manhattan(Pos{-1, 5}, Pos{1, 4}), 3);

  // --- Pos: == dan != ---
  CHECK(Pos{1, 2} == Pos{1, 2});
  CHECK(Pos{1, 2} != Pos{2, 1});
  CHECK(!(Pos{1, 2} != Pos{1, 2}));

  // --- Rng: seed sama = urutan sama, seed beda = urutan beda ---
  Rng a(123);
  Rng b(123);
  bool same = true;
  for (int i = 0; i < 20; ++i) {
    if (a() != b()) same = false;
  }
  CHECK(same);

  Rng c(1);
  Rng d(2);
  CHECK(c() != d());

  // --- Hierarki exception: semua turunan GameException (dan std::runtime_error) ---
  CHECK_THROWS(throwInvalidInput(), GameException);
  CHECK_THROWS(throwInvalidMove(), GameException);
  CHECK_THROWS(throwScoreFile(), GameException);
  CHECK_THROWS(throwInvalidInput(), InvalidInputException);
  CHECK_THROWS(throwInvalidMove(), InvalidMoveException);
  CHECK_THROWS(throwScoreFile(), ScoreFileException);
  CHECK_THROWS(throwInvalidInput(), std::runtime_error);

  // Pesan harus terbawa lewat what().
  std::string message;
  try {
    throwInvalidMove();
  } catch (const GameException& e) {
    message = e.what();
  }
  CHECK_EQ(message, std::string("gerak buruk"));

  return check_summary();
}
