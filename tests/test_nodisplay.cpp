// Tanpa display, game harus mencetak satu pesan Indonesia ke stderr dan
// keluar dengan kode 1 (bukan crash). Argumen 1: path executable game.
#include <sys/wait.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "check.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "pemakaian: test_nodisplay <path-game>\n";
    return 2;
  }
  const std::string errFile = "test_nodisplay_stderr.txt";
  const std::string cmd = "timeout 10 env -u DISPLAY -u WAYLAND_DISPLAY -u XDG_RUNTIME_DIR '" + std::string(argv[1]) +
                          "' >/dev/null 2>" + errFile;
  const int status = std::system(cmd.c_str());

  CHECK(WIFEXITED(status));  // timeout 10 -> kode 124 bila macet, tes gagal (tidak menggantung)
  CHECK_EQ(WEXITSTATUS(status), 1);

  std::ifstream in(errFile);
  std::stringstream buffer;
  buffer << in.rdbuf();
  const std::string err = buffer.str();
  CHECK(err.find("Galat: jendela grafis tidak bisa dibuat") != std::string::npos);
  std::remove(errFile.c_str());
  return check_summary();
}
