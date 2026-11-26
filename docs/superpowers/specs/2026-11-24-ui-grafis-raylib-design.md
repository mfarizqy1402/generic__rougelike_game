# Desain: UI grafis (raylib) menggantikan TUI FTXUI, dan ganti nama game

Status: DRAFT menunggu persetujuan. Semua keputusan di bawah adalah pilihan saya (Claude) kecuali yang bertanda "dari Teach".

## Maksud
Teach ingin game berjalan di jendela grafis, bukan terminal. Library harus ringan; tampilan disamakan dengan versi TUI yang sudah ada. Nama game menjadi `generic_roguelike_game` (ejaan persis dari Teach; "roguelike" mungkin salah ketik dari "roguelike", dipakai apa adanya kecuali Teach mengoreksi).

## Dari Teach
- Ganti FTXUI menjadi UI grafis dengan library ringan.
- Tampilan sama seperti yang sudah dibuat: peta berpetak, tembok blok penuh abu-abu, lantai kosong, zona aman berlatar hijau tua, musuh merah, HUD, log 4 baris, legenda 9 entri, menu, layar Game Over.
- Nama game: `generic_roguelike_game`.

## Tujuan
- Jendela grafis dengan tampilan setara (susunan, warna, teks Indonesia ASCII) dan kontrol sama (WASD/Panah, Spasi, 1-9, Esc).
- `core/` dan `controller/` TIDAK berubah dan tidak mengenal raylib.
- Semua logika layar tetap teruji tanpa jendela dan tanpa display (CI/ctest headless).

## Bukan tujuan
Animasi, suara efek, mouse di dalam peta, jendela yang bisa diubah ukuran, aset gambar/font eksternal, mengganti nama namespace `rogue` atau file `scores.txt`.

## Pendekatan yang dipertimbangkan
1. **raylib 5.5 (dipilih).** Satu library C, CMake FetchContent, jendela + gambar 2D + input dalam beberapa fungsi, tanpa aset. Sudah dicoba di sandbox: terbangun dan membuka jendela di Xvfb. Biaya: build Linux butuh paket dev X11/GL.
2. SDL3 (+ SDL3_ttf). Sama portabelnya tetapi lebih banyak kode dasar dan butuh file font; lebih berat untuk tugas ini.
3. SFML 3. Antarmuka C++ nyaman tetapi dependensi lebih banyak dan lebih besar.

## Keputusan desain
- **Audio:** raylib membawa miniaudio sendiri (0.11.21). Menautkannya bersama miniaudio kita (0.11.22) menimbulkan simbol ganda yang diam-diam memilih salinan raylib (sudah terbukti di sandbox). Maka raylib dibangun TANPA modul audio (`CUSTOMIZE_BUILD=ON`, `SUPPORT_MODULE_RAUDIO=OFF`), dan `MiniaudioPlayer` beserta tesnya tetap dipakai.
- **Pemisahan:** `src/ui/` menjadi murni (tanpa raylib): `ICanvas` (fillRect, strokeRect, text, textWidth), `InputFrame` (tombol yang baru ditekan, karakter yang diketik, posisi klik mouse), `Palette` (`ColorId` ke `Rgb`), `IScreen` baru (`update(const InputFrame&)`, `draw(ICanvas&) const`), `ScreenManager`, tiga layar, `Viewport`. `src/platform/` berisi satu-satunya kode raylib: `RaylibCanvas`, pembaca input, siklus jendela. Tes memakai `RecordingCanvas` (mencatat panggilan gambar, lebar teks palsu 8 px per karakter).
- **Jendela:** tetap 960x680, 60 FPS, Esc ditangani aplikasi (bukan tutup otomatis raylib), tombol X jendela keluar bersih. Log raylib dibatasi ke peringatan. Font bawaan raylib (ASCII, tanpa aset).
- **Susunan layar Game:** peta 17x15 petak, petak 32 px, simbol dipusatkan di petak memakai lebar teks terukur; HUD di kanan; log 4 baris, legenda 2 baris, dan baris bantuan di bawah. Warna sama dengan TUI.
- **Menu:** kolom Nama dan Seed (ketik langsung, hanya karakter ASCII yang diterima validator), Kesulitan (Kiri/Kanan), tombol Mulai/Keluar (Enter atau klik). Tab/Atas/Bawah pindah fokus. Tombol Mulai tersembunyi saat input tidak valid, seperti sebelumnya.
- **Nama:** `project(generic_roguelike_game)`, executable `generic_roguelike_game`, judul jendela, judul menu "G E N E R I C   R O G U E L I K E   G A M E", README, nama zip `generic_roguelike_game.zip`.
- **Tanpa display:** bila jendela gagal dibuat, program mencetak satu pesan Indonesia ke stderr dan keluar dengan kode 1 (tanpa crash).

## Diserahkan ke pelaksana
Ukuran tepat tata letak (selama semua teks muat dalam 960x680, diuji), nama berkas internal, urutan warna persis.

## Risiko
- Paket dev X11/GL tidak ada di mesin Teach (Arch: `libx11 libxrandr libxinerama libxcursor libxi mesa`); README mencatatnya.
- Tampilan sebenarnya hanya bisa dilihat Teach; di sandbox hanya diuji lewat Xvfb dan canvas pencatat. Build Windows/MSVC tidak diuji.
