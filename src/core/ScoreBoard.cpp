#include "core/ScoreBoard.hpp"

#include <algorithm>
#include <utility>

namespace rogue {

ScoreBoard::ScoreBoard(IScoreStore& store) : store_(store) {}

void ScoreBoard::normalize() {
  std::stable_sort(entries_.begin(), entries_.end(),
                   [](const ScoreEntry& a, const ScoreEntry& b) {
                     return a.score > b.score;
                   });
  if (entries_.size() > kMaxEntries) entries_.resize(kMaxEntries);
}

void ScoreBoard::reload() {
  std::vector<ScoreEntry> loaded = store_.load();  // bila melempar, entries_ utuh
  entries_ = std::move(loaded);
  normalize();
}

void ScoreBoard::add(const ScoreEntry& entry) {
  entries_.push_back(entry);
  normalize();
  store_.save(entries_);
}

const std::vector<ScoreEntry>& ScoreBoard::top() const { return entries_; }

}  // namespace rogue
