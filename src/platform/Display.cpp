#include "platform/Display.hpp"

// GLFW sudah ikut terkompilasi di dalam raylib; deklarasi minimal saja
// (header GLFW tidak diekspor oleh target raylib).
extern "C" {
int glfwInit(void);
void glfwTerminate(void);
void glfwWindowHint(int hint, int value);
void* glfwCreateWindow(int width, int height, const char* title, void* monitor, void* share);
void glfwDestroyWindow(void* window);
}

namespace rogue {

namespace {
// Nilai konstanta dari glfw3.h.
constexpr int kVisible = 0x00020004;
constexpr int kContextVersionMajor = 0x00022002;
constexpr int kContextVersionMinor = 0x00022003;
constexpr int kOpenGlForwardCompat = 0x00022006;
constexpr int kOpenGlProfile = 0x00022008;
constexpr int kOpenGlCoreProfile = 0x00032001;
}  // namespace

bool displayAvailable() {
  if (glfwInit() == 0) return false;

  // Coba buat jendela tersembunyi dengan konteks OpenGL 3.3 core, sama dengan
  // yang diminta raylib. Display ada tetapi tanpa GL 3.3 (VM, RDP, GPU lama)
  // juga membuat InitWindow abort, jadi harus terdeteksi di sini.
  glfwWindowHint(kVisible, 0);
  glfwWindowHint(kContextVersionMajor, 3);
  glfwWindowHint(kContextVersionMinor, 3);
  glfwWindowHint(kOpenGlProfile, kOpenGlCoreProfile);
#ifdef __APPLE__
  glfwWindowHint(kOpenGlForwardCompat, 1);
#else
  (void)kOpenGlForwardCompat;
#endif
  void* window = glfwCreateWindow(64, 64, "probe", nullptr, nullptr);
  const bool ok = window != nullptr;
  if (ok) glfwDestroyWindow(window);
  glfwTerminate();  // raylib menginisialisasi ulang (dan mengatur ulang hint) di InitWindow
  return ok;
}

}  // namespace rogue
