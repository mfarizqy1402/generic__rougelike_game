#include "platform/RaylibInput.hpp"

#include <raylib.h>

namespace rogue {

namespace {

bool pressedOrRepeated(int key) { return IsKeyPressed(key) || IsKeyPressedRepeat(key); }

}  // namespace

InputFrame readInput() {
  InputFrame frame;

  if (pressedOrRepeated(KEY_UP)) frame.pressed.push_back(Key::Up);
  if (pressedOrRepeated(KEY_DOWN)) frame.pressed.push_back(Key::Down);
  if (pressedOrRepeated(KEY_LEFT)) frame.pressed.push_back(Key::Left);
  if (pressedOrRepeated(KEY_RIGHT)) frame.pressed.push_back(Key::Right);
  if (pressedOrRepeated(KEY_BACKSPACE)) frame.pressed.push_back(Key::Backspace);
  if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) frame.pressed.push_back(Key::Enter);
  if (IsKeyPressed(KEY_TAB)) frame.pressed.push_back(Key::Tab);
  if (IsKeyPressed(KEY_ESCAPE)) frame.pressed.push_back(Key::Escape);

  for (int codepoint = GetCharPressed(); codepoint != 0; codepoint = GetCharPressed()) {
    if (codepoint >= 32 && codepoint <= 126) frame.typed += static_cast<char>(codepoint);
  }

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    frame.clicked = true;
    frame.mouseX = GetMouseX();
    frame.mouseY = GetMouseY();
  }
  return frame;
}

}  // namespace rogue
