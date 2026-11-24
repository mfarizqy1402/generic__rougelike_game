#pragma once

#include <string>

#include "core/IUsable.hpp"

namespace rogue {

// Kelas dasar ABSTRAK untuk barang; use() diwujudkan oleh subkelas.
class Item : public IUsable {
 public:
  explicit Item(std::string name);

  const std::string& name() const;

 private:
  std::string name_;
};

}  // namespace rogue
