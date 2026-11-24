# BGM dari folder `music/` (rancangan singkat)

Ukuran: perubahan sedang (dependensi baru, beberapa kelas baru, tanpa mengubah aturan game), jadi rancangan ini singkat.

## Tujuan dan alasan
- Game memutar semua file musik di folder `music/` (termasuk subfolder) sebagai latar suara selama game berjalan, supaya suasananya lebih hidup.

Bukan tujuan: tombol kontrol di dalam game (pause, volume, lewati lagu), pemutar daftar lagu di layar, format Ogg/Opus, pemutaran dari jaringan.
Anti-tujuan: game macet atau crash karena musik rusak, folder kosong, atau tidak ada perangkat suara; musik mengubah hasil seed game; layar menunggu musik; Esc tidak langsung keluar karena thread musik belum berhenti.

## Perilaku (diputuskan pengguna)
- Urutan lagu diacak setiap kali game dijalankan.
- Saat lagu habis, lagu berikutnya dari daftar acak diputar. Lagu yang sudah diputar masuk kembali ke antrian acak.
- Semua direktori musik di dalam `music/` ikut diputar (pemindaian rekursif).
- Pemutar memakai miniaudio (disetujui pengguna).

## Pendekatan
- miniaudio lewat FetchContent (dipilih pengguna): satu header, WAV/MP3/FLAC, lintas Linux dan Windows.
- Memanggil pemutar luar (`mpv`/`ffplay`) lewat `system()`: ditolak, bergantung pada program yang belum tentu terpasang.

## Rancangan
- **Folder dan format.** Folder `music/` dibuat di proyek dengan `music/README.txt` berisi petunjuk. Jalurnya relatif terhadap direktori kerja (sama seperti `scores.txt`). Ekstensi yang dipindai, tanpa membedakan huruf besar/kecil: `.mp3`, `.wav`, `.flac`.
- **`MusicLibrary`** (`src/core`, tanpa audio): `explicit MusicLibrary(std::filesystem::path dir)`; `std::vector<std::filesystem::path> scan() const` mengembalikan file musik secara rekursif, terurut, dan kosong bila folder tidak ada atau bukan direktori. Tidak pernah melempar karena folder hilang.
- **`ShuffleQueue`** (`src/core`, logika murni): berisi daftar lagu yang diacak dengan `Rng` miliknya sendiri (bukan `Rng` game, supaya seed game tidak terpengaruh). `next()` mengambil lagu terdepan, lalu menyisipkannya kembali di posisi acak di antara lagu lain, tidak di depan. Jadi lagu yang sama tidak diputar dua kali berturut-turut bila ada lebih dari satu lagu; dengan satu lagu, lagu itu berulang. Antrian kosong dilaporkan lewat `std::optional` kosong.
- **`IMusicPlayer`** (interface di `src/core`): `start()`, `stop()`, `std::string nowPlaying() const`. Dua implementasi:
  - `NullMusicPlayer`: tidak melakukan apa pun (dipakai bila tidak ada lagu atau perangkat suara gagal dibuka, dan di tes).
  - `MiniaudioPlayer` (`src/audio`): memutar lagu dari `ShuffleQueue`. Callback akhir lagu dari thread audio hanya menaikkan bendera; pemuatan lagu berikutnya dilakukan thread pengelola terpisah, bukan di callback. `stop()` menghentikan dan menunggu thread selesai; destruktor memanggilnya. File yang gagal dibuka dilewati, dan bila semua gagal pemutar berhenti diam-diam.
- **Perakitan** di `main.cpp`: membuat `MusicLibrary`, `ShuffleQueue` (diacak dengan `std::random_device`), memilih `MiniaudioPlayer` atau `NullMusicPlayer`, memanggil `start()` sebelum loop UI dan `stop()` setelahnya. `GameController` dan semua layar game tidak berubah.
- **Status di menu** (pilihan saya): satu baris kecil di `MenuScreen`, "Musik: N lagu" atau "Musik: tidak ada (isi folder music/)", lewat string baru di `AppContext`.
- **CMake.** miniaudio diunduh dengan `FetchContent` pada tag yang dipasang tetap. `MINIAUDIO_IMPLEMENTATION` dikompilasi di satu berkas `src/audio/miniaudio_impl.cpp` dengan peringatan dimatikan khusus berkas itu (berkas pihak ketiga), dan menautkan `pthread`, `dl`, `m` di Linux. Kelas audio tidak ikut `rogue_core` agar `core/` bebas dari miniaudio.

## Pengujian (ditulis sebelum kode)
- `ShuffleQueue`: semua lagu muncul; lagu yang sudah diputar kembali muncul; tidak ada pengulangan berurutan untuk 2 lagu atau lebih; satu lagu berulang; antrian kosong menghasilkan kosong; seed berbeda menghasilkan urutan berbeda.
- `MusicLibrary`: direktori sementara dengan subfolder, ekstensi huruf besar dan kecil, file bukan musik diabaikan, folder tidak ada atau berisi file biasa menghasilkan daftar kosong.
- `MiniaudioPlayer`: dengan backend null miniaudio dan dua file WAV pendek yang dibuat di tes, `nowPlaying()` berpindah ke lagu lain setelah lagu pertama habis, file rusak dilewati, dan `stop()` kembali cepat tanpa menggantung.
- Tidak bisa diuji di sandbox: suara yang keluar dari speaker. Itu diuji pengguna sendiri.

## Diserahkan ke pelaksana
- Tag miniaudio yang dipasang tetap dan opsi kompilasi yang dimatikan.
- Pola penghentian thread (mis. `std::condition_variable`) dan detail callback.
- Nama konstanta, dan apakah `NullMusicPlayer` dan `IMusicPlayer` berada dalam satu berkas.
- Pembaruan README (cara menaruh musik, format yang didukung, dependensi baru, pemetaan rubrik: `IMusicPlayer` interface, Null Object, `ShuffleQueue`).
