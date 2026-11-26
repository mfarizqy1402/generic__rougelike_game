#include "ui/ScreenManager.hpp"

#include <utility>

namespace rogue {

void ScreenManager::setFactory(Factory factory) { factory_ = std::move(factory); }

void ScreenManager::go(ScreenId id) { pending_ = id; }

bool ScreenManager::applyPending() {
  if (!pending_.has_value()) return false;
  const ScreenId id = *pending_;
  pending_.reset();
  if (!factory_) return false;

  std::unique_ptr<IScreen> next = factory_(id);
  if (!next) return false;

  retired_ = std::move(current_);
  current_ = std::move(next);
  return true;
}

IScreen* ScreenManager::current() const { return current_.get(); }

void ScreenManager::update(const InputFrame& input, const std::function<void()>& onEscape) {
  applyPending();
  if (input.pressedKey(Key::Escape)) {
    if (onEscape) onEscape();
    return;
  }
  if (current_ != nullptr) current_->update(input);
  applyPending();
}

void ScreenManager::draw(ICanvas& canvas) const {
  if (current_ != nullptr) current_->draw(canvas);
}

}  // namespace rogue
