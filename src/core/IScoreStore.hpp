#pragma once

#include <string>
#include <vector>

namespace rogue {

struct ScoreEntry {
  std::string name;
  int score = 0;
  std::string title;
};

// INTERFACE: tempat menyimpan papan skor. Implementasinya bisa berkas,
// memori, atau yang lain; ScoreBoard hanya bergantung pada antarmuka ini.
class IScoreStore {
 public:
  virtual ~IScoreStore() = default;
  virtual std::vector<ScoreEntry> load() = 0;
  virtual void save(const std::vector<ScoreEntry>& entries) = 0;
};

}  // namespace rogue
