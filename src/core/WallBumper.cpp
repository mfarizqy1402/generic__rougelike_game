#include "core/WallBumper.hpp"

namespace rogue {

WallBumper::WallBumper() : Achievement("Menabrak tembok 50 kali") {}

void WallBumper::handle(const GameEvent& event) {
  if (event.type != EventType::WallBump) return;
  ++bumps_;
  if (bumps_ >= kRequired) unlock();
}

void WallBumper::reset() { bumps_ = 0; }

}  // namespace rogue
