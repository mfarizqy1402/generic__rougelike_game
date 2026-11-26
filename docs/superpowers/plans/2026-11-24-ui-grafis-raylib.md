# UI Grafis raylib dan Ganti Nama Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ganti seluruh UI FTXUI menjadi jendela grafis raylib dengan tampilan setara, dan ganti nama game menjadi `generic_roguelike_game`.

**Architecture:** `src/ui/` dibuat murni (layar menggambar lewat `ICanvas`, membaca `InputFrame`), diuji dengan `RecordingCanvas` tanpa jendela. `src/platform/` adalah satu-satunya kode raylib (canvas, input, jendela). `core/`, `controller/`, `audio/` tidak berubah.

**Tech Stack:** C++17, CMake 3.16+, raylib 5.5 (FetchContent, modul audio MATI), miniaudio 0.11.22 (tetap), tes buatan sendiri `tests/check.hpp`.

**Spec:** `docs/superpowers/specs/2026-11-24-ui-grafis-raylib-design.md`

## Global Constraints
- `core/`, `controller/`, `audio/` tidak berubah perilaku; `grep -rnE "raylib|ftxui|miniaudio|rand\(|srand" src/core src/controller` kosong. `src/ui/` tidak boleh meng-include raylib; hanya `src/platform/` dan `src/main.cpp`.
- raylib: `GIT_TAG 5.5`, `CUSTOMIZE_BUILD ON`, `SUPPORT_MODULE_RAUDIO OFF`, `BUILD_EXAMPLES OFF` (agar tidak ada simbol miniaudio ganda; versi miniaudio yang tertaut harus 0.11.22).
- Jendela 960x680, 60 FPS, ASCII saja untuk teks di layar, teks Indonesia sama seperti TUI, kontrol sama (WASD/Panah, Spasi, 1-9, Esc).
- Nama: `generic_roguelike_game` persis (executable, project, judul jendela, README, zip `generic_roguelike_game.zip`). Namespace `rogue` dan `scores.txt` tetap.
- Tes tidak boleh butuh display. `kMaxInventory = 9`, `kViewW = 17`, `kViewH = 15`, `kLogLines = 4`.
- Warna: dinding abu-abu terang blok penuh, zona aman latar hijau tua, semua musuh merah, pemain cyan, peti hijau muda, tangga kuning, Batu oranye tua.
- `stop()` musik dan tutup jendela harus kembali cepat; kegagalan jendela = pesan + kode 1.

## Review Focus
1. Tanpa display (DISPLAY kosong): program mencetak satu pesan Indonesia ke stderr, keluar kode 1, tanpa crash (Task 6).
2. Teks muat di jendela 960x680 untuk kasus terburuk: tas 9 item, log 4 baris terpanjang, nama 16 karakter, 10 skor di papan (Task 3, 5).
3. Ketikan aneh di menu: kodepoin non-ASCII, karakter kontrol, melebihi panjang maks, Backspace saat kosong tidak merusak atau diterima (Task 4).
4. Esc dan tombol X jendela: keluar bersih, musik berhenti; Esc tidak menutup jendela otomatis oleh raylib (Task 2, 6).
5. Tombol ditahan (repeat) saat run berakhir: pindah ke Game Over tepat sekali dan input sesudahnya tidak melempar (Task 3).

---

### Task 1: ICanvas, InputFrame, Palette, RecordingCanvas, CMake raylib + nama

**Files:**
- Create: `src/ui/ICanvas.hpp`, `src/ui/InputFrame.hpp`, `src/ui/Palette.hpp`, `src/ui/Palette.cpp`, `tests/RecordingCanvas.hpp`, `tests/test_canvas.cpp`
- Modify: `CMakeLists.txt` (project name, FetchContent raylib menggantikan FTXUI, pustaka `rogue_ui` dari `src/ui/*.cpp` ditautkan ke `rogue_core`; `canvas` ditambah ke daftar tes)

**Interfaces:**
- Produces:
  - `struct Rgb { std::uint8_t r, g, b; };` (`operator==`)
  - `class ICanvas { virtual ~ICanvas()=default; virtual void fillRect(int x,int y,int w,int h,Rgb c)=0; virtual void strokeRect(int x,int y,int w,int h,Rgb c)=0; virtual void text(const std::string& s,int x,int y,int size,Rgb c)=0; virtual int textWidth(const std::string& s,int size) const=0; };`
  - `enum class Key { Up, Down, Left, Right, Enter, Escape, Tab, Backspace }; struct InputFrame { std::vector<Key> pressed; std::string typed; bool clicked=false; int mouseX=0; int mouseY=0; bool pressedKey(Key) const; };` (`typed` = karakter ASCII 32..126 yang diketik frame ini; WASD, spasi, 1-9 datang lewat `typed`)
  - `Rgb colorOf(ColorId)` di `Palette.hpp`: Default putih, Dim (192,192,192), Yellow (240,220,60), Cyan (60,220,230), Red (230,60,60), Brown (200,120,40), LightGreen (120,230,120), DarkGreen (0,90,40).
  - `tests/RecordingCanvas.hpp`: `RecordingCanvas : ICanvas` mencatat `std::vector<Call>` (`kind`, `x,y,w,h`, `text`, `Rgb`); `textWidth` = `8 * s.size()`; pembantu `bool hasText(const std::string&) const`, `std::vector<Call> rects(Rgb)`, `bool allInside(int w,int h) const`.

- [ ] **Step 1: Tes gagal** `tests/test_canvas.cpp`: `colorOf` mengembalikan nilai di atas untuk tiap `ColorId` dan Red/Cyan/DarkGreen saling berbeda; `InputFrame::pressedKey` benar/salah; `RecordingCanvas` mencatat `fillRect`/`text`, `hasText`, `rects(Rgb)`, `allInside(960,680)` true untuk teks di dalam dan false untuk teks yang lebarnya melewati batas kanan.
- [ ] **Step 2:** Bangun; harapkan gagal kompilasi (`ui/ICanvas.hpp` tidak ada).
- [ ] **Step 3:** Implementasi header/`Palette.cpp`. CMake: `project(generic_roguelike_game ...)`; blok FetchContent raylib (opsi di Global Constraints) menggantikan FTXUI; `rogue_ui` STATIC (kosongkan `target_link_libraries` raylib; hanya `rogue_core`); tes dipasang lewat `add_rogue_test(canvas)` dan ditautkan ke `rogue_ui`. Sumber `src/ui/*.cpp` lama yang masih memakai FTXUI dikeluarkan dari glob sementara (dihapus di Task 3-5).
- [ ] **Step 4:** `ctest -R canvas`; lalu pastikan konfigurasi CMake tidak menghasilkan simbol miniaudio ganda: biner kecil `ma_version_string()` yang ditautkan dengan `raylib` dan `miniaudio` mencetak `0.11.22`.

### Task 2: IScreen baru dan ScreenManager

**Files:**
- Modify: `src/ui/IScreen.hpp`, `src/ui/ScreenManager.hpp`, `src/ui/ScreenManager.cpp`, `src/ui/AppContext.hpp` (tambah `std::function<void()> quit` tetap; tidak ada lagi tipe FTXUI)
- Test: `tests/test_screenmanager.cpp` (didaftarkan di foreach)

**Interfaces:**
- Consumes: `InputFrame`, `ICanvas` (Task 1).
- Produces: `class IScreen { virtual ~IScreen()=default; virtual void update(const InputFrame&)=0; virtual void draw(ICanvas&) const=0; };`
  `class ScreenManager { using Factory = std::function<std::unique_ptr<IScreen>(ScreenId)>; void setFactory(Factory); void go(ScreenId); bool applyPending(); IScreen* current() const; void update(const InputFrame&, const std::function<void()>& onEscape); void draw(ICanvas&) const; };` `enum class ScreenId { Menu, Game, GameOver };` tetap. `update` menerapkan pergantian tertunda DULU, memanggil `onEscape` bila `Key::Escape` ditekan (dan tidak meneruskannya ke layar), selain itu meneruskan ke layar aktif lalu menerapkan pergantian yang baru dijadwalkan; `go()` tetap hanya menjadwalkan (aman dipanggil dari `update` layar).

- [ ] **Step 1: Tes gagal** dengan layar palsu yang menghitung `update`/`draw`: `go` tidak membuat layar sampai `applyPending`; `go` dari dalam `update` layar tidak menghancurkan layar yang sedang jalan; Escape memanggil `onEscape` tepat sekali dan tidak sampai ke layar; `draw` tanpa layar aktif tidak crash.
- [ ] **Step 2:** Bangun; gagal. **Step 3:** Implementasi (hapus kode FTXUI dan pemeriksaan ukuran terminal 80x24). **Step 4:** `ctest -R screenmanager` hijau.

### Task 3: GameScreen (peta, HUD, log, legenda, input)

**Files:**
- Modify: `src/ui/GameScreen.hpp`, `src/ui/GameScreen.cpp` (tulis ulang tanpa FTXUI); `src/ui/Viewport.*` tidak berubah
- Test: `tests/test_gamescreen.cpp` (tulis ulang memakai `RecordingCanvas`; hapus tes FTXUI lama, pertahankan maksud: HUD 9 item, tembok blok, lantai kosong, simbol terpusat, status menu dipindah ke Task 4)

**Interfaces:**
- Consumes: `buildViewport`, `legend()`, `colorOf`, `IScreen`, `GameController`, `AppContext` (`controller`, `screens`).
- Produces: `class GameScreen : public IScreen { explicit GameScreen(AppContext&); void update(const InputFrame&) override; void draw(ICanvas&) const override; };` Konstanta tata letak di `GameScreen.cpp`: jendela `kWindowW=960`, `kWindowH=680`; petak 32 px; peta di (16,16) berukuran 544x480 dengan bingkai; HUD mulai x=584 lebar 360; log 4 baris mulai y=508 (20 px per baris); legenda 2 baris mulai y=596 (22 px); baris bantuan y=656. Pelaksana boleh menggeser angka selama semua teks muat dan tidak saling menimpa.
  Perilaku: tembok = `fillRect` penuh Dim; petak zona aman = `fillRect` DarkGreen; simbol lain digambar `text` dengan warna `colorOf`, dipusatkan memakai `textWidth`; lantai biasa tidak menggambar apa pun; HUD memuat nama, "Lantai X   Skor Y", "HP a/b" + gauge (hijau > 50%, kuning > 25%, merah sesudahnya), status aktif, "Item:" dan hingga 9 baris "[n] nama"; log 4 baris terakhir; legenda 9 entri (musuh berwarna merah) memakai `legend()`; baris bantuan "WASD/Panah: gerak | Spasi: diam | 1-9: pakai item | Esc: keluar", atau pesan galat merah "! ..." bila aksi terakhir gagal. Satu tombol = satu panggilan controller: `typed` w/a/s/d (huruf besar sama) dan panah = `movePlayer`, spasi = `movePlayer(Dir::None)`, '1'-'9' = `useItem(n-1)`; `GameException` menjadi pesan; bila `isOver()` panggil `screens.go(ScreenId::GameOver)` tepat sekali dan abaikan input sesudahnya.

- [ ] **Step 1: Tes gagal** (`RecordingCanvas` 960x680, run seed "7", Mudah): (a) tembok digambar sebagai rect Dim berukuran 32x32 dan tidak ada teks "#"; (b) simbol pemain '@' berwarna Cyan, pusatnya (`x + textWidth/2`) sama dengan pusat petaknya (toleransi 1 px); (c) tas penuh 9 Potion + satu status: semua "[1]".."[9]" ada dan `allInside(960,680)`; (d) legenda memuat "Zona aman", "Goblin", "Tembok" dan label musuh berwarna Red; (e) pesan galat "! ..." muncul bila `useItem` pada slot kosong; (f) `InputFrame{typed="d"}` memindahkan pemain (controller dipanggil) dan `typed="5"` pada tas kosong memberi pesan, bukan crash; (g) Review Focus 5: paksa run selesai lalu kirim 3 `update` berturut-turut: `screens` dijadwalkan ke GameOver tepat sekali, tanpa pengecualian; (h) tata letak terburuk (log 4 baris terpanjang, nama 16 karakter) tetap `allInside`.
- [ ] **Step 2:** Bangun; gagal (konstruktor/`update` belum sesuai). **Step 3:** Tulis ulang `GameScreen`; hapus `#include` FTXUI. **Step 4:** `ctest -R gamescreen` hijau.

### Task 4: MenuScreen

**Files:**
- Modify: `src/ui/MenuScreen.hpp`, `src/ui/MenuScreen.cpp` (tulis ulang)
- Test: `tests/test_menuscreen.cpp` (didaftarkan di foreach)

**Interfaces:**
- Consumes: `InputValidator::validateName`, `InputValidator::parseSeed`, `AppContext` (`controller`, `screens`, `quit`, `lastName`, `lastDifficulty`, `musicStatus`).
- Produces: `class MenuScreen : public IScreen { explicit MenuScreen(AppContext&); void update(const InputFrame&) override; void draw(ICanvas&) const override; };` Fokus: Nama, Seed, Kesulitan, Mulai, Keluar. Tab/Bawah maju, Atas mundur (melingkar). Karakter `typed` masuk ke kolom yang fokus: Nama hanya `[A-Za-z0-9]` maks 16; Seed hanya digit maks 18; karakter lain diabaikan; Backspace menghapus satu karakter (aman saat kosong). Kiri/Kanan pada Kesulitan mengganti Mudah/Normal/Sulit (sesuai enum `Difficulty`). Enter pada Mulai (atau klik tombolnya) memanggil `controller.startRun(nama, seed, kesulitan)`, menyimpan `lastName`/`lastDifficulty`, lalu `screens.go(ScreenId::Game)`; `GameException` menjadi pesan galat merah. Tombol Mulai tidak digambar dan tidak aktif selama `validationMessage()` tidak kosong; pesan validasi tampil merah. Enter/klik pada Keluar memanggil `context.quit`. Judul "G E N E R I C   R O G U E L I K E   G A M E" kuning, tagline "Roguelike berbasis giliran. Tembok tidak minta maaf." redup, baris `musicStatus` redup (tidak ada bila kosong) di atas baris petunjuk "Tab/Atas/Bawah: pindah kolom  Kiri/Kanan: kesulitan  Enter: pilih  Esc: keluar".

- [ ] **Step 1: Tes gagal**: (a) judul memuat "GENERIC" dan "DUNGEON" tidak muncul; (b) mengetik "Teach" lalu Enter di Mulai (setelah fokus ke sana) memulai run dan menjadwalkan `ScreenId::Game`; (c) nama kosong/pendek: tidak ada teks "Mulai" dan Enter tidak memulai run; (d) Review Focus 3: ketik `"\xC3\xA9"` (non-ASCII), `"\x01"`, 40 huruf pada Nama (tersimpan 16), 40 digit pada Seed (tersimpan 18), Backspace 5 kali pada kolom kosong: nilai tetap valid dan tidak crash; (e) Kiri/Kanan mengganti kesulitan dan mengirimnya ke `startRun`; (f) klik di area tombol Keluar memanggil `quit`; (g) `musicStatus = "Musik: 3 lagu"` tampil, kosong tidak menampilkan "Musik:"; (h) `allInside(960,680)` termasuk pesan galat terpanjang.
- [ ] **Step 2:** Bangun; gagal. **Step 3:** Tulis ulang. **Step 4:** `ctest -R menuscreen` hijau.

### Task 5: GameOverScreen

**Files:**
- Modify: `src/ui/GameOverScreen.hpp`, `src/ui/GameOverScreen.cpp` (tulis ulang)
- Test: `tests/test_gameover.cpp` (didaftarkan di foreach)

**Interfaces:**
- Consumes: `RunSummary`, `ScoreBoard`, `TitleGenerator{}.titleFor(score, rng)`, `StatsTracker::get`, `Achievement`, `AppContext`.
- Produces: `class GameOverScreen : public IScreen { explicit GameOverScreen(AppContext&); void update(const InputFrame&) override; void draw(ICanvas&) const override; };` Konstruktor menyimpan skor seperti versi TUI (reload papan dulu, tambah entri, ambil `top()`), galat disimpan sebagai pesan merah. Dua panel: kiri (GAME OVER merah, Pemain, Skor, Lantai, Gelar kuning, Achievement dengan "(belum ada, sayang sekali)", Statistik 4 baris), kanan "Papan Skor" bernomor atau "(kosong)". Tombol "Main Lagi" dan "Menu": Kiri/Kanan/Tab pindah fokus, Enter atau klik memilih ("Main Lagi" = `startRun(lastName, "", lastDifficulty)` lalu Game; "Menu" = Menu). Petunjuk "Kiri/Kanan: pindah tombol   Enter: pilih   Esc: keluar".

- [ ] **Step 1: Tes gagal**: (a) teks "GAME OVER", skor, lantai, dan gelar tampil; (b) satu entri tersimpan di papan tepat sekali per layar dibuat; (c) achievement terbuka tampil sebagai "* judul"; (d) Enter pada "Main Lagi" memulai run baru dan menjadwalkan Game; Enter pada "Menu" menjadwalkan Menu; (e) papan 10 skor dengan nama 16 karakter: `allInside(960,680)` (Review Focus 2); (f) store yang gagal menyimpan menampilkan pesan, tidak crash.
- [ ] **Step 2:** Bangun; gagal. **Step 3:** Tulis ulang. **Step 4:** `ctest -R gameover` hijau.

### Task 6: Lapisan raylib dan main

**Files:**
- Create: `src/platform/RaylibCanvas.hpp`, `src/platform/RaylibCanvas.cpp`, `src/platform/RaylibInput.hpp`, `src/platform/RaylibInput.cpp`
- Modify: `src/main.cpp` (tulis ulang loop), `CMakeLists.txt` (target `generic_roguelike_game` = `main.cpp` + `src/platform/*.cpp`, tautkan `rogue_ui rogue_audio raylib`; hapus semua sisa FTXUI)
- Test: `tests/test_platform.cpp` (hanya diset bila `DISPLAY` ada, selain itu `return 0` dengan pesan SKIP)

**Interfaces:**
- Consumes: `ICanvas`, `InputFrame`, `ScreenManager`, `AppContext`, `IMusicPlayer`.
- Produces: `class RaylibCanvas : public ICanvas` (memakai `DrawRectangle`, `DrawRectangleLines`, `DrawText`, `MeasureText`); `InputFrame readInput();` (`IsKeyPressed` + `IsKeyPressedRepeat` untuk panah/Backspace, `GetCharPressed` disaring ke ASCII 32..126, klik kiri mouse); `main.cpp`: `SetTraceLogLevel(LOG_WARNING)`, `SetConfigFlags(FLAG_VSYNC_HINT)`, `InitWindow(960,680,"generic_roguelike_game")`, bila `!IsWindowReady()` cetak "Galat: jendela grafis tidak bisa dibuat (cek display/driver GL)." ke stderr dan `return 1`; `SetExitKey(KEY_NULL)`, `SetTargetFPS(60)`; `music->start()` sesudah jendela siap; loop `while(!WindowShouldClose() && !quitRequested)`: `screens.update(readInput(), context.quit)`, `BeginDrawing(); ClearBackground(<hitam>); screens.draw(canvas); EndDrawing();`; sesudah loop `music->stop()` lalu `CloseWindow()` (juga pada jalur exception). Perakitan musik dan `musicStatus` tidak berubah.

- [ ] **Step 1: Tes gagal** `test_platform.cpp` (di bawah `xvfb-run`): jendela tersembunyi (`FLAG_WINDOW_HIDDEN`) dibuat; `RaylibCanvas` menggambar satu frame untuk `MenuScreen`, `GameScreen`, `GameOverScreen` tanpa crash; `MeasureText("ABC", 20) > 0`. Bangun; gagal (berkas belum ada).
- [ ] **Step 2:** Implementasi. **Step 3:** `ctest` penuh hijau (tes platform SKIP tanpa display); `xvfb-run -a ctest -R platform` lulus.
- [ ] **Step 4:** E2E: `xvfb-run -a ./build/generic_roguelike_game` berjalan 3 detik lalu dihentikan dengan SIGTERM tanpa pesan galat; ambil tangkapan layar Xvfb (`import -window root` atau `xwd` bila ada) untuk Teach dan lihat sendiri menu dan layar Game; tanpa `DISPLAY` (`env -u DISPLAY ./build/generic_roguelike_game`) keluar kode 1 dengan pesan Indonesia (Review Focus 1); dengan Xvfb dan 2 WAV di `music/`, program tidak macet dan keluar bersih lewat SIGTERM dalam <= 1 s (Review Focus 4).

### Task 7: README, pembersihan, verifikasi, pengiriman

**Files:**
- Modify: `README.md`, `music/README.txt` (nama game), `.superpowers/` tidak diubah; Hapus: berkas FTXUI sisa, skrip pty lama tidak relevan
- Test: seluruh suite

- [ ] **Step 1:** README: nama baru, UI raylib, dependensi build (Arch: `sudo pacman -S --needed base-devel cmake git libx11 libxrandr libxinerama libxcursor libxi mesa`; Debian/Ubuntu: `libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev`; Windows tanpa paket tambahan), cara jalan (`./build/generic_roguelike_game` dari folder proyek), kontrol, pemetaan rubrik (`ICanvas` dan `IScreen` interface, `RaylibCanvas` Dependency Inversion, `RecordingCanvas` mudah diuji), jumlah tes, catatan: raylib dibangun tanpa audio agar tidak bentrok dengan miniaudio.
- [ ] **Step 2:** Verifikasi: `grep -rniE "ftxui" . --include=* -l` (selain `docs/` dan `.superpowers/`) kosong; `grep -rnE "raylib|ftxui|miniaudio|rand\(|srand" src/core src/controller` kosong; `grep -rn "raylib" src/ui` kosong; build bersih GCC dan Clang `-Wall -Wextra -Wpedantic` tanpa peringatan dari kode kita; `ctest` hijau; mutation singkat (mis. hapus pemusatan simbol, hapus penyaringan karakter non-ASCII, `onEscape` tidak dipanggil, `go(GameOver)` dua kali) dan tiap mutan harus membuat tes gagal.
- [ ] **Step 3:** Zip `generic_roguelike_game.zip` (tanpa `build/`, `.superpowers/`, `scores.txt`) dan kirim; nyatakan bahwa tampilan hanya diperiksa di Xvfb, build Windows/Arch belum diuji.
