#include "core/StatsTracker.hpp"

namespace rogue {

namespace {
constexpr int kFictionalMetersPerStep = 3;
}

StatsTracker::StatsTracker() { reset(); }

void StatsTracker::reset() {
  stats_ = {{"langkah", 0},
            {"meter_fiktif", 0},
            {"ragu_ragu", 0},
            {"tabrak_tembok", 0}};
}

void StatsTracker::onEvent(const GameEvent& event) {
  switch (event.type) {
    case EventType::RunStarted:
      reset();
      break;
    case EventType::Moved:
      ++stats_["langkah"];
      stats_["meter_fiktif"] += kFictionalMetersPerStep;
      break;
    case EventType::Waited:
      ++stats_["ragu_ragu"];
      break;
    case EventType::WallBump:
      ++stats_["tabrak_tembok"];
      break;
    default:
      break;
  }
}

const std::map<std::string, int>& StatsTracker::stats() const { return stats_; }

int StatsTracker::get(const std::string& key) const {
  const auto it = stats_.find(key);
  return it == stats_.end() ? 0 : it->second;
}

}  // namespace rogue
