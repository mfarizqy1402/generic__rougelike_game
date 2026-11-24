// Kerangka tes mini buatan sendiri (tanpa library tes eksternal).
// Pemakaian: tulis CHECK/CHECK_EQ/CHECK_THROWS di main(), lalu
// `return check_summary();` agar CTest tahu lulus atau gagal.
#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace check_detail {

inline int g_passed = 0;
inline int g_failed = 0;

// Tampilkan nilai di pesan galat bila tipenya bisa di-stream.
template <class T, class = void>
struct Streamable : std::false_type {};
template <class T>
struct Streamable<T, std::void_t<decltype(std::declval<std::ostream&>()
                                          << std::declval<const T&>())>>
    : std::true_type {};

template <class T>
std::string show(const T& value) {
  if constexpr (Streamable<T>::value) {
    std::ostringstream out;
    out << value;
    return out.str();
  } else {
    return "<tidak bisa dicetak>";
  }
}

inline void pass() { ++g_passed; }

inline void fail(const char* file, int line, const std::string& message) {
  ++g_failed;
  std::cerr << "[GAGAL] " << file << ":" << line << " " << message << "\n";
}

}  // namespace check_detail

// Variadic agar ekspresi seperti Pos{1, 2} == Pos{1, 2} (ada koma di dalam
// kurung kurawal) tidak terpecah menjadi beberapa argumen makro.
#define CHECK(...)                                                       \
  do {                                                                   \
    if (__VA_ARGS__) {                                                   \
      check_detail::pass();                                              \
    } else {                                                             \
      check_detail::fail(__FILE__, __LINE__, "CHECK(" #__VA_ARGS__ ")"); \
    }                                                                    \
  } while (0)

#define CHECK_EQ(a, b)                                                   \
  do {                                                                   \
    const auto& check_a_ = (a);                                          \
    const auto& check_b_ = (b);                                          \
    if (check_a_ == check_b_) {                                          \
      check_detail::pass();                                              \
    } else {                                                             \
      check_detail::fail(__FILE__, __LINE__,                             \
                         "CHECK_EQ(" #a ", " #b ") nilai kiri=" +        \
                             check_detail::show(check_a_) +              \
                             " kanan=" + check_detail::show(check_b_)); \
    }                                                                    \
  } while (0)

// CHECK_THROWS(ekspresi, TipeException): lulus bila melempar tipe itu.
#define CHECK_THROWS(expr, ExType)                                       \
  do {                                                                   \
    try {                                                                \
      (void)(expr);                                                      \
      check_detail::fail(__FILE__, __LINE__,                             \
                         "CHECK_THROWS(" #expr ") tidak melempar");      \
    } catch (const ExType&) {                                            \
      check_detail::pass();                                              \
    } catch (...) {                                                      \
      check_detail::fail(__FILE__, __LINE__,                             \
                         "CHECK_THROWS(" #expr ") melempar tipe lain");  \
    }                                                                    \
  } while (0)

// Cetak ringkasan; kembalikan 0 bila semua lulus, 1 bila ada yang gagal.
inline int check_summary() {
  std::cout << check_detail::g_passed << " lulus, " << check_detail::g_failed
            << " gagal\n";
  return check_detail::g_failed == 0 ? 0 : 1;
}
