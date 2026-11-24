# Ruang Loot, Peti, dan Legenda Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Tiap lantai bisa punya satu ruang loot (safe zone) berisi 1-3 peti satu-serangan yang memberi Ramuan, serta peta berwarna (musuh merah) dengan legenda simbol.

**Architecture:** `Tile` mendapat penanda `safe`; `DungeonGenerator` menandai satu ruang tengah. `Chest` adalah subkelas `Entity` pasif yang membawa item; `GameController` menaruhnya dan membukanya lewat jalur serangan yang sudah ada. `Viewport` mengurus warna dan `legend()` (logika murni, bisa diuji); `GameScreen` hanya menampilkannya.

**Tech Stack:** C++17, CMake, FTXUI v6.1.9, kerangka uji `tests/check.hpp`.

**Spec:** `docs/superpowers/specs/2026-11-22-ruang-loot-design.md`

## Global Constraints

- `core/` dan `controller/` tidak boleh memuat FTXUI, raylib, `rand()`, atau `srand`. Semua acak lewat `Rng` yang disuntikkan.
- Event handler UI hanya memanggil satu method `GameController` (tanpa logika bisnis).
- Layar harus muat di 80x24: peta 17 baris + log 4 baris + legenda 2 baris + bantuan 1 baris = 24.
- Teks di layar ASCII saja. Pesan dan komentar berbahasa Indonesia.
- Tes ditulis dan terbukti gagal sebelum kodenya. Perintah verifikasi proyek: `cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure`. Proyek ini bukan repositori git, jadi langkah "commit" diganti menjalankan seluruh `ctest` hijau.
- Keputusan spesifikasi: peti 1 HP; 0-1 ruang loot per lantai (peluang 60%); 1-3 peti per ruang; isi peti 1 Ramuan + 30% peluang 1 Gulungan; batas inventory 9 item; peti bukan kill dan tidak menambah skor.

## Review Focus

1. Lantai tanpa ruang tengah (cadangan 2 ruang): tidak ada ruang loot dan tidak crash.
2. Inventory 8 item + isi peti 2 item (jadi 10): serangan ditolak "Tas penuh", peti utuh, giliran tidak terpakai.
3. Musuh berdiri tepat di luar tepi safe zone saat pemain berdiri di tepi dalamnya: tetap bisa menyerang (aturan hanya melarang melangkah). Dilaporkan ke pengguna, bukan diubah diam-diam.
4. Hantu (tembus tembok) dan musuh yang sudah mengejar: tidak pernah berdiri di petak `safe`.
5. Pemain memasuki peti saat status `Hiccups` mengubah arah: tetap lewat jalur yang sama, tidak crash.

---

### Task 1: Tile.safe dan Dungeon.lootCenter

**Files:**
- Modify: `src/core/Tile.hpp`, `src/core/Tile.cpp`, `src/core/Dungeon.hpp`, `src/core/Dungeon.cpp`
- Test: `tests/test_dungeon.cpp`

**Interfaces:**
- Produces: `bool Tile::isSafe() const; void Tile::setSafe(bool);` (bawaan false). `void Dungeon::setLootCenter(Pos); std::optional<Pos> Dungeon::lootCenter() const;` (bawaan `nullopt`). Menyalin/memindahkan `Dungeon` ikut membawa nilainya.

- [ ] **Step 1: Tes gagal** di `test_dungeon.cpp`, fungsi `testSafeFlagAndLootCenter`:
```cpp
Dungeon d(12, 10);
CHECK(!d.tile(Pos{3, 3}).isSafe());
d.tile(Pos{3, 3}).setSafe(true);
CHECK(d.tile(Pos{3, 3}).isSafe());
CHECK(!d.lootCenter().has_value());
d.setLootCenter(Pos{4, 4});
CHECK(d.lootCenter().has_value() && *d.lootCenter() == (Pos{4, 4}));
Dungeon moved(std::move(d));
CHECK(moved.lootCenter().has_value() && moved.tile(Pos{3, 3}).isSafe());
```
Daftarkan di `main()`.
- [ ] **Step 2:** Jalankan build; harapkan gagal kompilasi (`isSafe` belum ada).
- [ ] **Step 3:** Tambah anggota `bool safe_ = false` pada `Tile` dan `std::optional<Pos> lootCenter_` pada `Dungeon`, beserta method di atas.
- [ ] **Step 4:** `ctest --test-dir build --output-on-failure`; harapkan semua lulus.

### Task 2: Entity Chest

**Files:**
- Create: `src/core/Chest.hpp`, `src/core/Chest.cpp` (CMake mengambil `src/core/*.cpp` otomatis; jalankan ulang `cmake -S . -B build`)
- Test: `tests/test_entities.cpp`

**Interfaces:**
- Consumes: `Entity(std::string name, int hp, Pos pos)`, `Item`, `Potion`, `Scroll`.
- Produces: `class Chest : public Entity` dengan `static constexpr int kHp = 1;`, `Chest(Pos pos, std::vector<std::shared_ptr<Item>> loot)`, `void act(Dungeon&, Rng&) override` (tidak melakukan apa pun), `const std::vector<std::shared_ptr<Item>>& loot() const`, `std::vector<std::shared_ptr<Item>> takeLoot()` (mengosongkan isi peti). Nama entity: `"Peti"`.

- [ ] **Step 1: Tes gagal** `testChest`:
```cpp
Chest c(Pos{2, 2}, {std::make_shared<Potion>()});
CHECK_EQ(c.name(), std::string("Peti"));
CHECK_EQ(c.hp(), 1);
CHECK(c.isAlive());
CHECK_EQ(c.loot().size(), std::size_t{1});
Dungeon d(12, 10); Rng rng(1);
c.act(d, rng);                       // tidak mengubah apa pun
CHECK(c.pos() == (Pos{2, 2}) && c.isAlive());
auto taken = c.takeLoot();
CHECK_EQ(taken.size(), std::size_t{1});
CHECK(c.loot().empty());
c.takeDamage(3);
CHECK(!c.isAlive());
```
- [ ] **Step 2:** Build; harapkan gagal (`Chest.hpp` tidak ada).
- [ ] **Step 3:** Implementasi sesuai Interfaces. `takeLoot` memakai `std::move` lalu `clear()`.
- [ ] **Step 4:** `ctest`; harapkan hijau.

### Task 3: Generator menandai ruang loot

**Files:**
- Modify: `src/core/DungeonGenerator.cpp` (konstanta `kLootRoomChance = 60`)
- Test: `tests/test_generator.cpp`

**Interfaces:**
- Consumes: `Tile::setSafe`, `Dungeon::setLootCenter` (Task 1), `randomBetween(rng, lo, hi)`.
- Produces: pada `generate`, setelah semua ruang dan koridor dibuat dan sebelum `setStart`/`setStairs` selesai, bila `rooms.size() >= 3` dan `randomBetween(rng, 1, 100) <= kLootRoomChance`, pilih indeks `randomBetween(rng, 1, rooms.size() - 2)`; tandai `safe` semua petak `Floor` di dalam persegi ruang itu dan panggil `setLootCenter(room.center())`. Layout ruang dan koridor untuk seed yang sama tidak berubah, karena lemparan acak baru dilakukan sesudahnya.

- [ ] **Step 1: Tes gagal** `testLootRoom` (seed 1..300, peta 40x25):
```cpp
// untuk tiap seed: kumpulkan petak safe
// - semua petak safe bertipe Floor
// - bila ada petak safe: lootCenter() ada, lootCenter safe, start() dan stairs() tidak safe
// - bila tidak ada petak safe: lootCenter() kosong
// - lantai dengan ruang loot ada dan yang tanpa juga ada (kedua jumlah > 0)
// - jumlah petak safe >= 12 (ruang minimum 4x3) bila ada
// - stairs tetap reachable dari start (pakai reachable() yang sudah ada)
```
Tambah tes `testLayoutUnchangedBySafeMarking`: untuk seed 1..50, tipe tiap petak sama dengan hasil sebelum fitur (bandingkan dengan generate yang `safe`-nya diabaikan: cukup periksa `type()` bukan `isSafe()`, dan jalankan dua kali dengan seed sama menghasilkan peta identik).
- [ ] **Step 2:** Build+`ctest`; harapkan gagal (tidak ada petak safe / lootCenter kosong).
- [ ] **Step 3:** Implementasi di `generate()`.
- [ ] **Step 4:** `ctest`; hijau. Sesuaikan tes lain bila ada yang rusak karena urutan acak lanjutan berubah, dan catat.

### Task 4: Musuh dilarang melangkah ke petak safe

**Files:**
- Modify: `src/core/Movement.hpp`, `src/core/Movement.cpp`, `src/core/Enemy.cpp`
- Test: `tests/test_entities.cpp`

**Interfaces:**
- Produces: `bool moveToward(Dungeon&, Entity& self, Pos target, bool throughWalls, bool avoidSafe = false);` Bila `avoidSafe` dan petak tujuan `isSafe()`, langkah itu dilewati (coba sumbu lain, lalu gagal). `Enemy::act` memanggil dengan `avoidSafe = true`; `PetRock` tidak berubah (default false). Pemanggil lama (4 argumen) tetap kompilasi.

- [ ] **Step 1: Tes gagal** `testEnemiesAvoidSafeTiles`:
```cpp
// peta lantai terbuka 12x10, petak (5,5) dan (6,5) safe
// Goblin di (4,5), target pemain di (8,5): setelah act() Goblin tetap di (4,5)
// moveToward(..., avoidSafe=true) mengembalikan false untuk langkah ke petak safe
// moveToward(..., avoidSafe=false) tetap masuk (perilaku lama)
// Hantu (throughWalls) di sisi tembok, tujuan petak safe: tetap tidak masuk
// PetRock mengikuti pemain masuk ke petak safe (boleh)
```
- [ ] **Step 2:** Build; harapkan gagal (parameter baru belum ada).
- [ ] **Step 3:** Implementasi; di `Enemy::act` teruskan `true`.
- [ ] **Step 4:** `ctest`; hijau (tes `moveToward` lama harus tetap lulus).

### Task 5: GameController menaruh peti dan melarang musuh muncul di safe zone

**Files:**
- Modify: `src/controller/GameController.hpp`, `src/controller/GameController.cpp`
- Test: `tests/test_controller.cpp`

**Interfaces:**
- Consumes: `Chest` (Task 2), `Dungeon::lootCenter`, `Tile::isSafe`.
- Produces: private `void placeChests();` dipanggil di `buildFloor` tepat setelah `placePetRock(start)` dan sebelum `placeEnemies(start)`. Bila `lootCenter()` kosong, tidak melakukan apa pun. Jika ada: `n = randomBetween(rng_, 1, 3)`; pilih `n` petak berbeda dari petak `safe` bertipe `Floor` yang bukan `lootCenter` dan belum ditempati; tiap peti memuat `Potion`, ditambah `Scroll` bila `randomBetween(rng_, 1, 100) <= 30`. `placeEnemies` melewati petak `isSafe()`.

- [ ] **Step 1: Tes gagal** (cari seed 1..300 lewat `startRun`):
```cpp
// testChestsPlacedInLootRoom: bila lootCenter ada -> 1..3 Chest, semua di petak safe,
//   tidak ada yang di lootCenter, posisi saling berbeda, tiap peti punya >=1 Ramuan
// testNoChestsWithoutLootRoom: lootCenter kosong -> 0 Chest
// testBothFloorKindsOccur: dari seed-seed itu ada lantai dengan dan tanpa ruang loot
// testNoEnemySpawnsInSafeZone: tidak ada Enemy di petak safe saat lantai dibangun (Sulit, banyak seed)
// testLootRoomAlsoOnLaterFloors: turun tangga (const_cast dungeon + pindahkan pemain ke tangga)
//   lantai 2 juga bisa punya peti pada sebagian seed
```
- [ ] **Step 2:** Build+`ctest`; harapkan gagal.
- [ ] **Step 3:** Implementasi `placeChests` dan filter di `placeEnemies`. Konstanta baru di namespace anonim: `kMaxInventory = 9`, `kChestScrollChance = 30`.
- [ ] **Step 4:** `ctest`; hijau. Tes lama yang bergantung pada urutan acak disesuaikan dan dicatat.

### Task 6: Membuka peti dan event ChestOpened

**Files:**
- Modify: `src/core/GameEvent.hpp` (enum `EventType` tambah `ChestOpened`, dokumentasikan: detail daftar nama item, value jumlah item), `src/controller/GameController.hpp/.cpp`, `src/core/StatsTracker.cpp` dan `src/core/Achievement.cpp` bila ada `switch` yang perlu cabang baru
- Test: `tests/test_controller.cpp`

**Interfaces:**
- Produces: private `void openChest(Player& me, Chest& chest);`. Di `movePlayer`, tepat sebelum cabang tukar tempat: `if (Chest* chest = dynamic_cast<Chest*>(occupant)) { ... }`.
  - Jika `me.inventory().size() + chest.loot().size() > kMaxInventory`: log `"Tas penuh. Peti tidak bisa dibuka."`, peti utuh, **tidak** memanggil `endTurn`, tidak memancarkan event.
  - Jika tidak: `chest.takeDamage(kPlayerAttackDamage)`; peti dihapus dari `dungeon_->entities()`; isi masuk ke inventory; log `"Kamu membuka Peti. Isinya: <nama item dipisah koma>."`; `emit(EventType::ChestOpened, <nama item dipisah koma>, <jumlah>)`; lalu `endTurn(true)`.
  - Tidak memanggil `++kills_`, tidak memancarkan `EnemyDied` atau `Attacked`; `steps_` tidak berubah.

- [ ] **Step 1: Tes gagal** (taruh `Chest` di sebelah pemain lewat `const_cast`):
```cpp
// testOpeningChestGivesItemAndRemovesChest: inventory +1 Ramuan, peti hilang dari entities,
//   log memuat "Peti", event ChestOpened tercatat tepat 1 kali dengan value == 1
// testChestIsNotAKill: summary().kills dan score tidak berubah, tidak ada EnemyDied/Attacked,
//   pemain tidak berpindah tempat
// testChestOpeningUsesATurn: musuh di dekatnya bertindak (fase musuh jalan)
// testFullInventoryRejectsChest: inventory 9 item -> peti tetap hidup, log "Tas penuh",
//   tidak ada event, steps/giliran tidak maju (posisi musuh tidak berubah)
// testNearlyFullInventoryRejectsTwoItemChest: inventory 8 + peti 2 item -> ditolak;
//   inventory 7 + peti 2 item -> diterima (9 item)
// testChestOpensViaHiccupsRedirect: dengan status Hiccups pemain tetap tidak crash
//   saat peti ada di sekitarnya (50 langkah acak)
// testObserversStillWork: StatsTracker dan Pacifist tidak berubah akibat membuka peti
```
- [ ] **Step 2:** Build+`ctest`; harapkan gagal.
- [ ] **Step 3:** Implementasi. Perbarui `switch` yang peringatan `-Wswitch` bila ada.
- [ ] **Step 4:** `ctest`; hijau. Bangun dengan GCC dan Clang `-Wall -Wextra -Wpedantic`; harapkan nol warning.

### Task 7: Viewport: warna, glyph peti, lantai aman, legend()

**Files:**
- Modify: `src/ui/Viewport.hpp`, `src/ui/Viewport.cpp`
- Test: `tests/test_viewport.cpp`

**Interfaces:**
- Produces: `enum class ColorId` ditambah `Red`, `DarkGreen`. Warna entity: `g`, `s`, `H` semuanya `Red`; `@` `Cyan`; `o` `Brown`; `C` (Chest) `LightGreen`. Petak `Floor` yang `isSafe()` digambar `'.'` `DarkGreen`; selain itu seperti sebelumnya.
  `struct LegendItem { char glyph; ColorId color; std::string label; bool enemy; };` dan `std::vector<LegendItem> legend();` dengan urutan: `@` "Kamu", `g` "Goblin", `s` "Slime", `H` "Hantu", `o` "Batu", `C` "Peti", `>` "Tangga", `.` DarkGreen "Zona aman". `enemy` true hanya untuk g, s, H.

- [ ] **Step 1: Tes gagal** (ubah dan tambah di `test_viewport.cpp`):
```cpp
// perbarui tes warna lama: g, s, H -> ColorId::Red (glyph tetap)
// perbarui tes tile: '.' Dim hanya untuk petak non-safe; petak safe '.' DarkGreen
// testChestGlyph: Chest di dalam jangkauan viewport -> 'C' LightGreen
// testEnemiesRedOthersNot: semua LegendItem.enemy memakai Red; tak satu pun non-enemy memakai Red
// testLegendCoversAllGlyphs: setiap glyph non-spasi yang muncul di buildViewport untuk
//   seed 1..30 (selain '#') ada di legend() (cocokkan glyph+warna), dan legend() tidak kosong
//   serta tiap label tidak kosong
```
- [ ] **Step 2:** Build+`ctest`; harapkan gagal.
- [ ] **Step 3:** Implementasi. `cellForEntity` mengenali `Chest` lewat `dynamic_cast`. Tetap tanpa FTXUI.
- [ ] **Step 4:** `ctest`; hijau.

### Task 8: GameScreen menampilkan legenda, README, dan verifikasi akhir

**Files:**
- Modify: `src/ui/GameScreen.hpp`, `src/ui/GameScreen.cpp` (pemetaan `Red -> ftxui::Color::Red`, `DarkGreen -> ftxui::Color::DarkGreen`; `kLogLines = 4`; tambah `renderLegend()`), `README.md`
- Test: pty (tanpa unit test; bagian logikanya sudah diuji di Task 7)

**Interfaces:**
- Consumes: `legend()` dan `ColorId` (Task 7).
- Produces: `ftxui::Element GameScreen::renderLegend() const;` menyusun 8 entri `legend()` menjadi 2 baris (4 entri per baris), tiap entri `glyph` berwarna + spasi + label. Tata letak vertikal: `{top, renderLog(), renderLegend(), footer}` = 17+4+2+1 = 24 baris.

- [ ] **Step 1:** Implementasi di `GameScreen`.
- [ ] **Step 2:** Bangun, lalu jalankan `build/roguelike` di pty 100x30 dengan `pyte` (skrip serupa `/tmp/e2e/run.py`): mulai dari menu dengan seed yang punya ruang loot (cari seed dengan `./build/test_*` atau skrip kecil). Periksa layar: legenda dua baris terlihat, total tinggi tidak melebihi 24 baris, glyph `C` terlihat di ruang loot, menabrak peti menambah item di panel Item, dan pesan "Tas penuh" muncul saat inventory 9.
- [ ] **Step 3:** Perbarui README: pemetaan rubrik (`Chest` subkelas `Entity` lain, `Tile.safe`, event `ChestOpened`), legenda tombol/peta, jumlah tes.
- [ ] **Step 4:** Verifikasi akhir: `ctest` hijau dengan GCC dan Clang tanpa warning; pemindaian `grep -rnE "ftxui|raylib|rand\(|srand" src/core src/controller` kosong; kerjakan mutation check singkat (mis. hapus larangan `avoidSafe`, ubah batas `> kMaxInventory`, ubah `kHp`) dan pastikan tes gagal pada tiap mutan; buat `rogue-uas-lootroom.zip` tanpa `build/` dan kirim.
