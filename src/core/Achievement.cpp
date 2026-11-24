#include "core/Achievement.hpp"

#include <utility>

namespace rogue {

Achievement::Achievement(std::string title) : title_(std::move(title)) {}

const std::string& Achievement::title() const { return title_; }
bool Achievement::unlocked() const { return unlocked_; }

void Achievement::onEvent(const GameEvent& event) {
  if (event.type == EventType::RunStarted) {
    unlocked_ = false;
    reset();
    return;
  }
  handle(event);
}

void Achievement::unlock() { unlocked_ = true; }

}  // namespace rogue
