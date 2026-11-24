#include "core/Pacifist.hpp"

namespace rogue {

Pacifist::Pacifist() : Achievement("Menunggu 20 kali tanpa alasan") {}

void Pacifist::handle(const GameEvent& event) {
  if (event.type != EventType::Waited || event.detail != "diam") return;
  ++waits_;
  if (waits_ >= kRequired) unlock();
}

void Pacifist::reset() { waits_ = 0; }

}  // namespace rogue
