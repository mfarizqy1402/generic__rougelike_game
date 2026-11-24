cmake_minimum_required(VERSION 3.16)
project(rogue_uas LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

# Peringatan kompiler yang sama untuk semua target.
function(rogue_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

# ---------------------------------------------------------------
# Pustaka logika game (core + controller). TANPA FTXUI.
# ---------------------------------------------------------------
file(GLOB ROGUE_CORE_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/core/*.cpp
  ${CMAKE_SOURCE_DIR}/src/controller/*.cpp)

add_library(rogue_core STATIC ${ROGUE_CORE_SOURCES})
target_include_directories(rogue_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
rogue_warnings(rogue_core)

# ---------------------------------------------------------------
# Tes: satu executable per tests/test_<nama>.cpp, didaftarkan ke CTest.
# ---------------------------------------------------------------
function(add_rogue_test name)
  add_executable(test_${name} ${CMAKE_SOURCE_DIR}/tests/test_${name}.cpp)
  target_link_libraries(test_${name} PRIVATE rogue_core)
  target_include_directories(test_${name} PRIVATE ${CMAKE_SOURCE_DIR}/tests)
  rogue_warnings(test_${name})
  add_test(NAME ${name} COMMAND test_${name})
endfunction()

# Tes didaftarkan hanya bila berkasnya sudah ada (dikirim bertahap per batch).
foreach(t smoke dungeon generator entities status_items controller extras scores music)
  if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_${t}.cpp)
    add_rogue_test(${t})
  endif()
endforeach()

# Tes viewport memakai ui/Viewport.cpp (tanpa FTXUI), jadi disusun terpisah.
if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_viewport.cpp)
  add_rogue_test(viewport)
  target_sources(test_viewport PRIVATE ${CMAKE_SOURCE_DIR}/src/ui/Viewport.cpp)
endif()

# ---------------------------------------------------------------
# Aplikasi TUI (FTXUI lewat FetchContent). Baru ada sejak Batch 5.
# ---------------------------------------------------------------
if(EXISTS ${CMAKE_SOURCE_DIR}/src/main.cpp)
  include(FetchContent)
  set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
    GIT_TAG v6.1.9)
  FetchContent_MakeAvailable(ftxui)

  # Pemutar musik latar (miniaudio, satu header, tanpa dependensi sistem tambahan).
  set(MINIAUDIO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(MINIAUDIO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio
    GIT_TAG 0.11.22)
  FetchContent_MakeAvailable(miniaudio)

  file(GLOB ROGUE_AUDIO_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/audio/*.cpp)
  add_library(rogue_audio STATIC ${ROGUE_AUDIO_SOURCES})
  target_include_directories(rogue_audio PUBLIC ${CMAKE_SOURCE_DIR}/src)
  find_package(Threads REQUIRED)
  target_link_libraries(rogue_audio PUBLIC rogue_core Threads::Threads ${CMAKE_DL_LIBS}
    PRIVATE miniaudio)
  if(UNIX)
    target_link_libraries(rogue_audio PUBLIC m)
  endif()
  rogue_warnings(rogue_audio)

  # Tes pemutar musik memakai backend null miniaudio.
  if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_player.cpp)
    add_rogue_test(player)
    target_link_libraries(test_player PRIVATE rogue_audio)
  endif()

  file(GLOB ROGUE_UI_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/ui/*.cpp)
  add_executable(roguelike ${CMAKE_SOURCE_DIR}/src/main.cpp ${ROGUE_UI_SOURCES})
  target_link_libraries(roguelike PRIVATE rogue_core rogue_audio
    ftxui::screen ftxui::dom ftxui::component)
  rogue_warnings(roguelike)

  # Tes tampilan GameScreen (butuh FTXUI, jadi disusun di sini).
  if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_gamescreen.cpp)
    add_rogue_test(gamescreen)
    target_sources(test_gamescreen PRIVATE ${ROGUE_UI_SOURCES})
    target_link_libraries(test_gamescreen PRIVATE ftxui::screen ftxui::dom ftxui::component)
  endif()
endif()
