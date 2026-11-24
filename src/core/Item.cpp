#include "core/Item.hpp"

#include <utility>

namespace rogue {

Item::Item(std::string name) : name_(std::move(name)) {}

const std::string& Item::name() const { return name_; }

}  // namespace rogue
