#include "core/ShuffleQueue.hpp"

#include <algorithm>
#include <iterator>
#include <random>
#include <utility>

namespace rogue {

ShuffleQueue::ShuffleQueue(std::vector<std::filesystem::path> items, Rng rng)
    : rng_(std::move(rng)) {
  std::shuffle(items.begin(), items.end(), rng_);
  items_.assign(std::make_move_iterator(items.begin()),
                std::make_move_iterator(items.end()));
}

std::optional<std::filesystem::path> ShuffleQueue::next() {
  if (items_.empty()) return std::nullopt;
  std::filesystem::path song = std::move(items_.front());
  items_.pop_front();
  const int remaining = static_cast<int>(items_.size());
  if (remaining == 0) {
    items_.push_back(song);
  } else {
    // Indeks 1..remaining: indeks 0 (depan) tidak pernah dipilih.
    const int index = std::uniform_int_distribution<int>(1, remaining)(rng_);
    items_.insert(items_.begin() + index, song);
  }
  return song;
}

}  // namespace rogue
