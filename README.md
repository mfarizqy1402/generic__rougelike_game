# generic_roguelike_game

Roguelike berbasis giliran dengan antarmuka grafis (jendela 960x680). Proyek UAS Pemrograman Berorientasi Objek.
Bahasa C++17, UI memakai [raylib](https://github.com/raysan5/raylib) 5.5 (ringan, diunduh otomatis oleh CMake), build dengan CMake.

## Cara bermain

Jelajahi lantai berbentuk labirin, hindari musuh, ambil item, dan turun lewat tangga `>`.

| Tombol | Fungsi |
|---|---|
| `W A S D` atau panah | Bergerak |
| Spasi | Diam satu giliran |
| `1`-`9` | Pakai item nomor tersebut |
| `Esc` | Keluar |

Di menu: ketik langsung di kolom Nama/Seed, `Tab` atau panah atas/bawah pindah kolom, kiri/kanan mengganti kesulitan, `Enter` memilih tombol (tombol juga bisa diklik mouse).
Nama harus 3-16 karakter (huruf, angka, spasi). Seed berupa angka (maksimal 9 digit); kosongkan untuk acak.
Seed yang sama menghasilkan run yang sama pada build yang sama.

Legenda peta (juga tampil di bawah peta saat bermain): `@` kamu, `g` goblin, `s` slime, `H` hantu (semua musuh berwarna terakota), `o` batu peliharaan, `C` peti, `>` tangga, blok abu-biru `██` = tembok, petak berlatar hijau lumut = zona aman. Lantai biasa dibiarkan kosong.

Tema tampilan: Frutiger Eco (aqua, hijau daun, langit mutiara, tanpa warna neon) dengan gaya Y2K seperti pemutar media retro: jendela ber-title bar aqua kilap, panel kaca, tombol perak bevel, bar HP seperti slider. Warna ada di `src/ui/Palette.cpp` dan `src/ui/Theme.cpp`; ada tes yang menjaga palet tetap bebas neon dan teksnya terbaca (kontras).

Ruang loot: sebagian lantai (sekitar 60%) punya satu ruang zona aman berisi 1-3 peti. Musuh tidak bisa melangkah masuk ke sana. Serang peti (jalan ke arahnya) untuk memecahkannya; isinya (Ramuan, kadang Gulungan Oleng) masuk ke inventory. Inventory maksimal 9 item; bila tidak muat, peti tidak bisa dibuka.
Skor tersimpan di `scores.txt` pada direktori kerja.

## Build dan jalankan

Butuh compiler C++17, CMake 3.16+, git, dan koneksi internet saat konfigurasi pertama (untuk mengunduh raylib dan miniaudio). Jendela grafis butuh display; tanpa display game menampilkan pesan galat dan keluar. Suara memakai ALSA/PulseAudio/JACK bila ada. Tanpa perangkat suara, game tetap jalan tanpa musik. raylib dibangun TANPA modul audio bawaannya supaya tidak bentrok dengan miniaudio yang dipakai pemutar musik.

### Arch Linux

Paket untuk Debian/Ubuntu: `build-essential cmake git libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev`.

```bash
sudo pacman -S --needed base-devel cmake git libx11 libxrandr libxinerama libxcursor libxi mesa
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/generic_roguelike_game
```

### Windows

Buka *Developer PowerShell for VS* (MSVC) atau terminal MinGW dengan CMake dan git terpasang.

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\generic_roguelike_game.exe     # MSVC
.\build\generic_roguelike_game.exe             # MinGW (generator tunggal-konfigurasi)
```

Di Windows tidak perlu paket tambahan (raylib memakai OpenGL bawaan sistem).
Catatan: build di Arch dan Windows belum dicoba oleh penulis kode; pengujian dilakukan dengan GCC 13 dan Clang di Linux.

## Musik

Letakkan berkas `.mp3`, `.wav`, atau `.flac` di folder `music/` (boleh di subfolder). Saat game dijalankan, semua lagu diacak dan diputar sebagai BGM. Bila sebuah lagu selesai, lagu acak berikutnya diputar; lagu yang sudah diputar disisipkan kembali ke antrian acak (tidak pernah langsung diulang bila ada lagu lain). Tidak ada kontrol musik di dalam game. Jalankan game dari folder proyek agar `music/` ditemukan. Folder kosong, berkas rusak, atau tidak ada perangkat suara tidak membuat game gagal; menu menampilkan status musik.

## Struktur

```
src/core/        model domain (tanpa UI)
src/controller/  GameController dan InputValidator (tanpa UI)
src/ui/          layar, ICanvas, InputFrame, Palette, Theme (widget bergaya Y2K), Viewport (murni, tanpa raylib)
src/platform/    RaylibCanvas, RaylibInput, Display (satu-satunya kode raylib)
src/audio/       MiniaudioPlayer (satu-satunya kode yang memakai miniaudio)
music/           folder lagu BGM
src/main.cpp     perakitan objek
tests/           test_*.cpp, kerangka uji kecil di check.hpp
```

Aliran satu arah: `ui` memanggil `controller`, `controller` memakai `core`. `core` dan `controller` tidak bergantung pada raylib. `ui` hanya mengenal antarmuka `ICanvas` dan `InputFrame`; `platform` mengimplementasikannya dengan raylib (Dependency Inversion), sehingga semua layar bisa diuji tanpa jendela.
Event handler UI tidak berisi logika bisnis; setiap tombol hanya memanggil satu method `GameController`.

## Pemetaan rubrik ke kelas

### Kelas domain (minimal 6)
`Dungeon`, `Tile`, `Entity`, `Player`, `Enemy`, `Goblin`, `Slime`, `Ghost`, `PetRock`, `Chest`, `Item`, `Potion`, `Scroll`, `StatusEffect`, `Hiccups`, `Overconfident`, `Achievement`, `StatsTracker`, `ScoreBoard`.

### Relasi antar kelas
| Relasi | Contoh |
|---|---|
| Asosiasi | `GameController` menggunakan `Dungeon`, `FortuneTeller`, dan `InputValidator` |
| Agregasi | `ScoreBoard` meminjam `IScoreStore&`; `AppContext` meminjam controller, board, stats |
| Komposisi | `Dungeon` memiliki `Tile` dan seluruh `Entity` (`vector<unique_ptr<Entity>>`); `Chest` memiliki isinya (`vector<shared_ptr<Item>>`); `GameController` memiliki `Dungeon` dan `Rng` |

### Prinsip OOP
| Prinsip | Bukti |
|---|---|
| Enkapsulasi | Data private/protected dengan akses lewat method, mis. `Entity` (HP lewat `takeDamage`), `Dungeon` (grid tidak terbuka) |
| Inheritance | `Entity` -> `Player`, `Enemy`, `PetRock`, `Chest`; `Enemy` -> `Goblin`, `Slime`, `Ghost`; `Item` -> `Potion`, `Scroll`; `StatusEffect` -> `Hiccups`, `Overconfident`; `Achievement` -> `WallBumper`, `SlimeVictim`, `Pacifist` |
| Polimorfisme | `act()`, `taunt()`, `takeDamage()`, `use()`, `onEvent()` dipanggil lewat pointer/referensi kelas dasar |

### Kelas abstrak dan interface
- Kelas abstrak: `Entity`, `Enemy`, `StatusEffect`, `Item`, `Achievement`.
- Interface (semua method murni virtual): `IUsable`, `IGameObserver`, `IScoreStore`, `IScreen`, `ICanvas`, `IMusicPlayer`.

### SOLID
| Prinsip | Penerapan |
|---|---|
| S | `InputValidator` hanya memvalidasi; `DungeonGenerator` hanya membangun peta; `Viewport` hanya menyusun grid tampilan; `MusicLibrary` hanya memindai folder; `ShuffleQueue` hanya mengatur urutan lagu |
| O | Musuh, item, status, achievement baru cukup subclass baru tanpa mengubah `GameController` |
| L | Semua `Enemy` bisa dipakai di tempat `Enemy`; semua `IScoreStore` bisa dipakai `ScoreBoard` |
| I | Interface kecil: `IUsable` (satu method), `IGameObserver`, `IScoreStore`, `IScreen` |
| D | `ScoreBoard` bergantung pada `IScoreStore`, bukan `FileScoreStore`; `Rng` dan observer disuntikkan; layar bergantung pada `GameController`, bukan sebaliknya; `main` memakai `IMusicPlayer`, bukan `MiniaudioPlayer` langsung |

### Pola desain
- Observer: `GameController` memancarkan `GameEvent` (termasuk `ChestOpened`) ke `IGameObserver` (`StatsTracker`, `Achievement`).
- Null Object: `NullMusicPlayer` dipakai bila `music/` kosong, sehingga `main` tidak perlu memeriksa nullptr.
- Template Method: `Achievement::onEvent` bersifat `final` dan memanggil `handle` dan `reset`.

### Exception handling
Hierarki `GameException` (turunan `std::runtime_error`): `InvalidInputException`, `InvalidMoveException`, `ScoreFileException`.
Layar menangkapnya dan menampilkan pesan, bukan crash. Contoh: input nama salah, aksi sebelum run dimulai atau setelah game over, berkas skor gagal ditulis.

### Koleksi STL
`std::vector` (tile, entity, inventory, papan skor), `std::deque` (log pesan), `std::map` (statistik), `std::optional` (seed), `std::unique_ptr` dan `std::shared_ptr` (kepemilikan).

### UI minimal 3 layar dan validasi input
- Layar: `MenuScreen`, `GameScreen`, `GameOverScreen`, dikelola `ScreenManager`.
- Validasi: `InputValidator::validateName` dan `parseSeed`; tombol Mulai tersembunyi selama input tidak valid.

## Pengujian

Delapan belas executable uji, dijalankan `ctest`: smoke, dungeon, generator, entities, status_items, controller, extras, scores, music, viewport, canvas, screenmanager, gamescreen, menuscreen, gameover, player, platform, nodisplay.
- Layar (`gamescreen`, `menuscreen`, `gameover`, `screenmanager`) diuji dengan `RecordingCanvas`, kanvas palsu yang mencatat panggilan gambar: tanpa jendela, tanpa display.
- `music` menguji `ShuffleQueue`, `MusicLibrary`, `NullMusicPlayer`; `player` menguji `MiniaudioPlayer` dengan backend null miniaudio (tanpa speaker).
- `platform` menggambar layar sungguhan ke jendela tersembunyi dan butuh display; tanpa display tes melaporkan SKIP. Jalankan `xvfb-run -a ctest --test-dir build -R platform` di mesin headless.
- `nodisplay` memastikan game tanpa display keluar dengan kode 1 dan pesan galat, bukan crash.
