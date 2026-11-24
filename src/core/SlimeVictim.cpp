#include "core/SlimeVictim.hpp"

namespace rogue {

SlimeVictim::SlimeVictim() : Achievement("Mati karena slime") {}

void SlimeVictim::handle(const GameEvent& event) {
  if (event.type == EventType::PlayerDied && event.detail == "Slime") unlock();
}

// Tidak punya hitungan sendiri; status terbuka sudah direset oleh kelas dasar.
void SlimeVictim::reset() {}

}  // namespace rogue
