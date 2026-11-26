cmake_minimum_required(VERSION 3.16)
project(generic_roguelike_game LANGUAGES C CXX)

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
# Pustaka logika game (core + controller). TANPA UI.
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

# ---------------------------------------------------------------
# Pustaka UI murni (layar, canvas, viewport): TANPA raylib, jadi bisa diuji
# tanpa jendela dan tanpa display.
# ---------------------------------------------------------------
add_library(rogue_ui STATIC
  ${CMAKE_SOURCE_DIR}/src/ui/GameOverScreen.cpp
  ${CMAKE_SOURCE_DIR}/src/ui/GameScreen.cpp
  ${CMAKE_SOURCE_DIR}/src/ui/MenuScreen.cpp
  ${CMAKE_SOURCE_DIR}/src/ui/Palette.cpp
  ${CMAKE_SOURCE_DIR}/src/ui/ScreenManager.cpp
  ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
  ${CMAKE_SOURCE_DIR}/src/ui/Viewport.cpp)
target_include_directories(rogue_ui PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(rogue_ui PUBLIC rogue_core)
rogue_warnings(rogue_ui)

foreach(t viewport canvas theme screenmanager gamescreen menuscreen gameover)
  if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_${t}.cpp)
    add_rogue_test(${t})
    target_link_libraries(test_${t} PRIVATE rogue_ui)
  endif()
endforeach()

# ---------------------------------------------------------------
# Dependensi eksternal: miniaudio (musik) dan raylib (jendela grafis).
# raylib dibangun TANPA modul audio supaya tidak bentrok dengan miniaudio kita.
# ---------------------------------------------------------------
include(FetchContent)

set(MINIAUDIO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MINIAUDIO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(miniaudio
  GIT_REPOSITORY https://github.com/mackron/miniaudio
  GIT_TAG 0.11.22)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
set(SUPPORT_MODULE_RAUDIO OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib
  GIT_TAG 5.5)

FetchContent_MakeAvailable(miniaudio raylib)

# Pemutar musik latar (miniaudio).
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

# ---------------------------------------------------------------
# Lapisan platform (satu-satunya kode raylib) dan program utama.
# ---------------------------------------------------------------
add_library(rogue_platform STATIC
  ${CMAKE_SOURCE_DIR}/src/platform/Display.cpp
  ${CMAKE_SOURCE_DIR}/src/platform/RaylibCanvas.cpp
  ${CMAKE_SOURCE_DIR}/src/platform/RaylibInput.cpp)
target_include_directories(rogue_platform PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(rogue_platform PUBLIC rogue_ui raylib)
rogue_warnings(rogue_platform)

# Tes platform butuh display; tanpa display tes melaporkan SKIP.
add_rogue_test(platform)
target_link_libraries(test_platform PRIVATE rogue_platform)

add_executable(generic_roguelike_game ${CMAKE_SOURCE_DIR}/src/main.cpp)
target_link_libraries(generic_roguelike_game PRIVATE rogue_platform rogue_ui rogue_audio)
rogue_warnings(generic_roguelike_game)

# Tanpa display: pesan galat dan kode keluar 1 (bukan crash). Hanya Linux/BSD
# (di macOS display selalu ada).
if(UNIX AND NOT APPLE)
  add_executable(test_nodisplay ${CMAKE_SOURCE_DIR}/tests/test_nodisplay.cpp)
  target_include_directories(test_nodisplay PRIVATE ${CMAKE_SOURCE_DIR}/tests)
  rogue_warnings(test_nodisplay)
  add_test(NAME nodisplay COMMAND test_nodisplay $<TARGET_FILE:generic_roguelike_game>)
endif()
