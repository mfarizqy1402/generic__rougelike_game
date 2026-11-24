#pragma once

#include <random>

namespace rogue {

// Satu generator acak untuk seluruh game. Dimiliki GameController dan
// disuntikkan (injected) ke semua kode core, sehingga seed yang sama
// menghasilkan run yang identik.
using Rng = std::mt19937;

}  // namespace rogue
