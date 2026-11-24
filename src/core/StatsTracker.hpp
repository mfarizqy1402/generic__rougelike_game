#pragma once

#include <map>
#include <string>

#include "core/GameEvent.hpp"
#include "core/IGameObserver.hpp"

namespace rogue {

// Pengamat yang menghitung statistik absurd satu run.
// Kunci: "langkah", "meter_fiktif" (3 per langkah), "ragu_ragu" (jumlah
// Waited), "tabrak_tembok". Semua kembali ke 0 saat RunStarted.
class StatsTracker : public IGameObserver {
 public:
  StatsTracker();

  void onEvent(const GameEvent& event) override;

  const std::map<std::string, int>& stats() const;

  // Nilai satu kunci; 0 bila kuncinya tidak dikenal.
  int get(const std::string& key) const;

 private:
  void reset();

  std::map<std::string, int> stats_;
};

}  // namespace rogue
