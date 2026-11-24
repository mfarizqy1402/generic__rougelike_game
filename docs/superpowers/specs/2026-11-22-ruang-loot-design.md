# Ruang loot, peti, dan legenda peta (rancangan singkat)

Ukuran: perubahan kecil sampai sedang pada proyek yang sudah ada, jadi rancangan ini singkat.

## Tujuan dan alasan
- Pemain sekarang hanya mendapat 1 Ramuan dan 1 Gulungan di awal run, lalu kehabisan. Ruang loot memberi ramuan baru selama permainan.
- Pemain bisa membedakan musuh dari yang bukan musuh dan tahu arti tiap simbol di peta.

Bukan tujuan: ekonomi item yang rumit, toko, item baru, atau peti yang bisa dikunci.
Anti-tujuan: game jadi terlalu mudah sampai ramuan tidak berarti, atau peti memblokir jalan ke tangga.

## Pendekatan yang dipertimbangkan
- Item tergeletak sebagai daftar di `Dungeon`: bersih, tapi bukan yang diminta.
- Item sebagai `Entity` lain yang pasif: dipilih, dengan bentuk peti yang bisa diserang.
- Item disimpan di `Tile`: mencampur peta dengan isi.
- Safe zone: musuh tidak muncul di sana saja (kurang aman) atau musuh dilarang melangkah ke sana (dipilih).

Dipilih oleh pengguna: peti sebagai `Entity` yang diserang, dengan item masuk ke pemain saat peti mati. Safe zone lewat larangan melangkah.

## Rancangan

**Ruang loot (Tile dan generator)**
- `Tile` mendapat penanda `safe`. Hanya petak lantai di dalam ruang loot yang ditandai.
- Per lantai ada 0 atau 1 ruang loot. Peluang ada: 60% (pilihan saya). Ruang dipilih dari ruang yang bukan ruang awal dan bukan ruang tangga; kalau tidak ada ruang tengah, lantai itu tanpa ruang loot.
- Penandaan dilakukan `DungeonGenerator` memakai `Rng` yang sama. Urutan acak berubah, jadi peta untuk seed lama ikut berubah.

**Peti**
- `Chest` adalah subkelas `Entity` (bukan `Enemy`), HP 1, `act()` tidak melakukan apa-apa.
- Satu ruang loot berisi 1 sampai 3 peti, ditempatkan `GameController` pada petak `safe` yang berbeda dan bukan titik tengah ruang (titik tengah dilewati koridor).
- Isi tiap peti: 1 Ramuan, ditambah 30% peluang 1 Gulungan (pilihan saya).

**Membuka peti (`GameController::movePlayer`)**
- Bila petak tujuan berisi `Chest`, pemain menyerangnya. Ini memakai satu giliran, sama seperti menyerang musuh. Peti mati dengan satu serangan.
- Peti dihapus dari peta, item masuk ke inventory, log mencatatnya, dan event baru `ChestOpened` dipancarkan.
- Peti tidak dihitung sebagai kill, tidak menambah skor, dan tidak memicu `EnemyDied`.
- Bila inventory sudah 9 item, serangan ke peti ditolak: muncul pesan "Tas penuh", peti tetap utuh, giliran tidak terpakai. Batas 9 karena hanya tombol 1-9 yang bisa memakai item.
- Karena peti hilang setelah satu serangan, peti tidak bisa memblokir jalan.

**Safe zone**
- Musuh tidak muncul di petak `safe` (`placeEnemies`).
- Musuh dilarang melangkah ke petak `safe`, termasuk Hantu yang menembus tembok. `PetRock` tetap boleh masuk.

**Tampilan**
- Warna: semua musuh merah (`g`, `s`, `H`); yang bukan musuh berwarna lain: `@` cyan, `o` cokelat, `C` hijau terang.
- Lantai safe zone digambar `.` hijau gelap agar ruangnya terlihat (pilihan saya).
- Legenda: `Viewport` menyediakan `legend()` berisi simbol, warna, nama, dan penanda musuh. `GameScreen` menampilkannya dalam 2 baris. Log dipendekkan dari 6 menjadi 4 baris supaya total tetap 24 baris.
- Entri legenda: `@` Kamu, `g` Goblin, `s` Slime, `H` Hantu, `o` Batu, `C` Peti, `>` Tangga, `.` hijau zona aman.

## Pengujian (ditulis sebelum kode)
- `test_entities`: `Chest` hidup dengan HP 1, `takeDamage` mematikannya, `act()` tidak mengubah apa pun.
- `test_generator`: petak `safe` hanya lantai, tidak ada di ruang awal dan tangga, 0 atau 1 ruang per lantai untuk banyak seed, dan keduanya muncul.
- `test_controller`: menyerang peti membuka dan memberi item; peti bukan kill dan tidak menambah skor; inventory penuh menolak tanpa memakai giliran; event `ChestOpened`; musuh tidak pernah berdiri di petak `safe` selama banyak giliran; musuh tidak muncul di sana; `PetRock` boleh masuk.
- `test_viewport`: warna musuh merah dan bukan musuh bukan merah; glyph `C`; petak safe berwarna; setiap glyph yang bisa tampil ada di `legend()`.
- Tes lama yang bergantung pada urutan acak disesuaikan bila perlu dan dilaporkan.
- Layar FTXUI tetap diuji lewat terminal semu: legenda terlihat dan layar tetap muat di 80x24.

## Diserahkan ke pelaksana
- Pemilihan ruang loot (mis. ruang pertama yang memenuhi syarat atau acak) dan batas ukuran ruang agar muat 3 peti.
- Nama konstanta dan struktur kecil `LegendItem`.
- Pembaruan README (pemetaan rubrik: `Chest` sebagai subkelas `Entity` lain).
