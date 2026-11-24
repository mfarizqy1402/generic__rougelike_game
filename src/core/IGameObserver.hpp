#pragma once

#include "core/GameEvent.hpp"

namespace rogue {

// INTERFACE (pola Observer): pihak yang ingin tahu kejadian di dalam game,
// misalnya pencatat statistik dan achievement.
class IGameObserver {
 public:
  virtual ~IGameObserver() = default;
  virtual void onEvent(const GameEvent& event) = 0;
};

}  // namespace rogue
