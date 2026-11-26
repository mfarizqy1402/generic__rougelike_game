#pragma once

namespace rogue {

// True bila jendela grafis dengan konteks OpenGL 3.3 bisa dibuat.
// raylib 5.5 tidak berhenti dengan rapi saat GLFW atau pembuatan konteks gagal
// (InitWindow abort/segfault), jadi dicoba dulu dengan jendela probe
// tersembunyi sebelum InitWindow.
bool displayAvailable();

}  // namespace rogue
