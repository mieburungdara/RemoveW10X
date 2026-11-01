# RemoveW10X

English · [**Bahasa Indonesia**](README.id.md)

Menghilangkan tombol close pada **jendela aplikasi lain** di Windows 10/11 x64.
Satu file exe kecil — tanpa installer, tanpa dependency runtime.

![Ikon](bukti/icon.png)

## Menjalankan

Unduh [`RemoveW10X.exe`](RemoveW10X.exe) lalu klik dua kali. Tidak perlu instalasi.

SmartScreen mungkin memberi peringatan saat pertama kali dijalankan karena binary
ini tidak bertanda tangan — pilih **More info → Run anyway**.

## Fungsinya

Pilih satu jendela dari daftar, lalu klik aksi. Kedua aksi bisa dibatalkan dengan
tombol **Kembalikan**.

| Tombol | Mekanisme | Hasil |
|---|---|---|
| **Hilangkan tombol X saja** | menghapus item `SC_CLOSE` dari system menu target | hanya tombol close yang jadi abu-abu; minimize dan maximize tetap hidup |
| **Hilangkan semua tombol caption** | mematikan style jendela `WS_SYSMENU` | minimize, maximize, **dan** close hilang bersamaan |

Terukur pada jendela yang benar-benar terbuka: jumlah item system menu berubah
`7 → 6` dan caption target langsung menggambar ulang tombol close yang mati.

## Batasannya — baca ini sebelum mengandalkannya

- **Yang dihentikan hanya KLIK pada tombol.** `Alt+F4`, "Close window" dari taskbar,
  dan jalur penutupan bawaan aplikasi target tetap menutup jendela. Ini diukur:
  setelah X kami hilangkan, `WM_SYSCOMMAND / SC_CLOSE` yang dikirim ke jendela itu
  tetap membuatnya mati. Mengunci total butuh message hook yang disuntikkan ke proses
  target — dan sengaja tidak dilakukan aplikasi ini.
- **Jendela elevated ditolak.** Proses non-admin tidak boleh mengubah jendela milik
  proses admin. Aplikasi melaporkan kegagalannya, tidak berpura-pura berhasil.
  Jalankan sebagai administrator kalau memang perlu.
- **Aplikasi yang menggambar title bar sendiri** (Chrome, Discord, aplikasi Electron)
  tidak punya caption Win32 untuk diubah. Aplikasi ini mengatakannya per jendela,
  bukan diam-diam tidak melakukan apa-apa.
- Sebagian jendela mengembalikan handle system menu yang hanya sah di dalam proses
  target (`err 1401`). Kalau begitu, aplikasi mereset menu lebih dulu untuk mendapat
  salinan yang bisa dipakai lalu mencoba lagi; jika tetap gagal, Anda diarahkan ke
  mode kedua.

## Bahasa

Dua tombol bendera di kanan atas mengganti seluruh tampilan — label, tombol, dan
log — antara Bahasa Indonesia dan Inggris. Defaultnya mengikuti bahasa UI Windows
Anda. Benderanya digambar dengan GDI karena GDI Win32 tidak merender emoji bendera
sebagai satu glif.

## Build dari sumber

Perlu MinGW-w64 `g++` di `PATH` (tidak butuh MSVC):

```bat
build.bat
```

Proses build memakai `-static` dengan sengaja: di mesin ini ada dua instalasi MinGW
di `PATH`, dan binary yang di-link dinamis mati saat dimuat dengan `0xC0000139`
(entry point tidak ditemukan). Hasil build statis jalan di mana saja.

## Berkas

| Berkas | Peran |
|---|---|
| `main.cpp` | UI, daftar jendela, dua mode, i18n |
| `closebtn.h` | pemanggilan Win32 sebenarnya + pembacaan status yang jujur |
| `winfind.h` | enumerasi dan pencarian jendela tingkat atas |
| `app.rc`, `app.ico` | ikon exe (dibuat `iconmaker.cpp`) |
| `iconmaker.cpp` | menggambar ikon pada 7 ukuran dan mengemasnya jadi ICO valid |
| `probe.cpp` | harness CLI: state / hide / gray / restore / nostyle / tryclose |
| `snap.cpp` | menangkap jendela dengan `PrintWindow` ke PNG |
| `drive.cpp` | mengklik tombol aplikasi ini sendiri lalu menDump lognya, untuk pengujian |
| `bukti/` | bukti visual kedua mode dan ikon |

Harness di atas itulah yang dipakai mengukur semua klaim di halaman ini, contohnya:

```bat
probe.exe state "Untitled - Notepad"
probe.exe hide  "Untitled - Notepad"
probe.exe tryclose "Untitled - Notepad"
```

`RemoveW10X.exe` ikut di-commit supaya orang bisa langsung menjalankan. Kalau Anda
mengedit sumber lalu build ulang, binary-nya akan muncul sebagai diff di git.
