#pragma once

#include "ui/ICanvas.hpp"
#include "ui/InputFrame.hpp"

namespace rogue {

// INTERFACE sebuah layar. Tiap frame: update() menerima masukan, lalu draw()
// menggambar lewat ICanvas. Layar tidak mengenal raylib.
class IScreen {
 public:
  virtual ~IScreen() = default;
  virtual void update(const InputFrame& input) = 0;
  virtual void draw(ICanvas& canvas) const = 0;
};

}  // namespace rogue
