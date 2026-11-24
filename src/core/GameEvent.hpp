#pragma once

#include <string>

namespace rogue {

enum class EventType {
  Moved,
  WallBump,
  Waited,
  Attacked,
  EnemyDied,
  PlayerDied,
  StatusApplied,
  FloorChanged,
  ChestOpened,
  RunStarted
};

// Kejadian di dalam game yang dikirim ke para pengamat (Observer).
//   Moved         : -
//   WallBump      : -
//   Waited        : detail "diam" (diminta pemain) atau "tertahan" (dibatalkan status)
//   Attacked      : detail nama penyerang, value damage
//   EnemyDied     : detail nama musuh
//   PlayerDied    : detail nama penyerang
//   StatusApplied : detail nama status
//   FloorChanged  : value nomor lantai baru
//   ChestOpened   : detail nama item (dipisah koma), value jumlah item
//   RunStarted    : detail nama pemain
struct GameEvent {
  EventType type;
  std::string detail;
  int value = 0;
};

}  // namespace rogue
