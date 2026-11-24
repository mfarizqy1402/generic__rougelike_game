# Generic Rougelike Game

Roguelike berbasis giliran dengan antarmuka terminal (TUI). Proyek UAS Pemrograman Berorientasi Objek.
Bahasa C++17, UI memakai [FTXUI](https://github.com/ArthurSonzogni/FTXUI) v6.1.9 (diunduh otomatis oleh CMake), build dengan CMake.

## Cara bermain

Jelajahi lantai berbentuk labirin, hindari musuh, ambil item, dan turun lewat tangga `>`.

| Tombol | Fungsi |
|---|---|
| `W A S D` atau panah | Bergerak |
| Spasi | Diam satu giliran |
| `1`-`9` | Pakai item nomor tersebut |
| `Esc` | Keluar |

Di menu: panah atas/bawah pindah kolom, kiri/kanan mengganti kesulitan, `Enter` menekan tombol.
Nama harus 3-16 karakter (huruf, angka, spasi). Seed berupa angka (maksimal 9 digit); kosongkan untuk acak.
Seed yang sama menghasilkan run yang sama pada build yang sama.

Legenda peta (juga tampil di bawah peta saat bermain): `@` kamu, `g` goblin, `s` slime, `H` hantu (semua musuh berwarna merah), `o` batu peliharaan, `C` peti, `>` tangga, blok abu-abu `██` = tembok, petak berlatar hijau = zona aman. Lantai biasa dibiarkan kosong.

Ruang loot: sebagian lantai (sekitar 60%) punya satu ruang zona aman berisi 1-3 peti. Musuh tidak bisa melangkah masuk ke sana. Serang peti (jalan ke arahnya) untuk memecahkannya; isinya (Ramuan, kadang Gulungan) masuk ke inventory. Inventory maksimal 9 item; bila tidak muat, peti tidak bisa dibuka.
Terminal minimal 80x24. Skor tersimpan di `scores.txt` pada direktori kerja.

## Build dan jalankan

Butuh compiler C++17, CMake 3.16+, git, dan koneksi internet saat konfigurasi pertama (untuk mengunduh FTXUI dan miniaudio). Suara memakai ALSA/PulseAudio/JACK bila ada; untuk build tidak perlu paket tambahan. Tanpa perangkat suara, game tetap jalan tanpa musik.

### Arch Linux

```bash
sudo pacman -S --needed base-devel cmake git
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/roguelike
```

### Windows

Buka *Developer PowerShell for VS* (MSVC) atau terminal MinGW dengan CMake dan git terpasang.

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\roguelike.exe     # MSVC
.\build\roguelike.exe             # MinGW (generator tunggal-konfigurasi)
```

Gunakan Windows Terminal agar warna dan karakter garis tampil benar.
Catatan: build di Arch dan Windows belum dicoba oleh penulis kode; pengujian dilakukan dengan GCC 13 dan Clang di Linux.

## Musik

Letakkan berkas `.mp3`, `.wav`, atau `.flac` di folder `music/` (boleh di subfolder). Saat game dijalankan, semua lagu diacak dan diputar sebagai BGM. Bila sebuah lagu selesai, lagu acak berikutnya diputar; lagu yang sudah diputar disisipkan kembali ke antrian acak (tidak pernah langsung diulang bila ada lagu lain). Tidak ada kontrol musik di dalam game. Jalankan game dari folder proyek agar `music/` ditemukan. Folder kosong, berkas rusak, atau tidak ada perangkat suara tidak membuat game gagal; menu menampilkan status musik.

## Struktur

```
src/core/        model domain (tanpa UI, tanpa FTXUI)
src/controller/  GameController dan InputValidator (tanpa FTXUI)
src/ui/          layar FTXUI dan Viewport
src/audio/       MiniaudioPlayer (satu-satunya kode yang memakai miniaudio)
music/           folder lagu BGM
src/main.cpp     perakitan objek
tests/           test_*.cpp, kerangka uji kecil di check.hpp
```

Aliran satu arah: `ui` memanggil `controller`, `controller` memakai `core`. `core` dan `controller` tidak bergantung pada FTXUI.
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
- Interface (semua method murni virtual): `IUsable`, `IGameObserver`, `IScoreStore`, `IScreen`, `IMusicPlayer`.

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

### TUI minimal 3 layar dan validasi input
- Layar: `MenuScreen`, `GameScreen`, `GameOverScreen`, dikelola `ScreenManager`.
- Validasi: `InputValidator::validateName` dan `parseSeed`; tombol Mulai tersembunyi selama input tidak valid.

## Pengujian

Dua belas executable uji, dijalankan `ctest`: smoke, dungeon, generator, entities, status_items, controller, extras, scores, music, player, viewport, gamescreen.
`music` menguji `ShuffleQueue`, `MusicLibrary`, `NullMusicPlayer`; `player` menguji `MiniaudioPlayer` dengan backend null miniaudio (tanpa speaker).
`gamescreen` merender layar permainan ke layar semu 80x24. Menu dan layar Game Over diuji dengan menjalankan program di terminal semu.
