// probe.cpp - harness verifikasi (dev tool, bukan bagian dari aplikasi).
// Tujuan: membuktikan apakah manipulasi system menu jendela milik proses lain
// benar-benar terjadi, bukan sekadar "GDI call returns TRUE".
#include <windows.h>

#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>

#include "closebtn.h"
#include "winfind.h"

// Cetak lebar sebagai UTF-8 supaya output bisa di-pipe ke bash.
// Catatan: UCRT wide-printf memakai %ls untuk wchar_t*, %s untuk char*.
static void Out(const wchar_t* fmt, ...) {
    wchar_t buf[4096];
    va_list ap;
    va_start(ap, fmt);
    int n = vswprintf(buf, 4096, fmt, ap);
    va_end(ap);
    if (n <= 0) return;
    int bytes = WideCharToMultiByte(CP_UTF8, 0, buf, n, nullptr, 0, nullptr, nullptr);
    if (bytes > 0) {
        std::string u(bytes, '\0');
        WideCharToMultiByte(CP_UTF8, 0, buf, n, u.data(), bytes, nullptr, nullptr);
        fwrite(u.data(), 1, u.size(), stdout);
    }
    fflush(stdout);
}

using Readout = MenuProbe;
static Readout ReadClose(HWND hwnd) { return ProbeClose(hwnd); }
static std::wstring Describe(const Readout& r) { return DescribeMenu(r); }

static void Dump(const wchar_t* label, HWND hwnd) {
    Readout r = ReadClose(hwnd);
    Out(L"%ls: hwnd=%p title=\"%ls\" proc=%ls\n", label, (void*)hwnd, WindowTitle(hwnd).c_str(),
        ProcName(hwnd).c_str());
    Out(L"     SC_CLOSE = %ls\n", Describe(r).c_str());
}

// Uji fungsional: kirim SC_CLOSE seperti saat tombol X diklik. Kalau benar-benar
// disable, DefWindowProc target harus mengabaikannya dan jendela tetap hidup.
static bool TryClose(HWND hwnd) {
    DWORD_PTR res = 0;
    SendMessageTimeoutW(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0, SMTO_NORMAL, 2000, &res);
    Sleep(400);
    return IsWindow(hwnd) != 0;
}

// Uji jalur klik caption TANPA menggerakkan mouse asli (sesi pengguna sedang
// dipakai). WM_NCLBUTTONDOWN dengan wParam=HTCLOSE melewati jalur kode yang sama
// dengan klik pada tombol X: target mengirim WM_SYSCOMMAND SC_CLOSE ke dirinya
// sendiri, dan DefWindowProc memeriksa status item menu sebelum menutup.
static int ClickClose(HWND hwnd, bool doClick) {
    RECT rc{};
    if (!GetWindowRect(hwnd, &rc)) return 2;
    int x = 0, y = 0;
    bool located = false;
    for (int dx = 12; dx <= 140; dx += 4) {
        int px = rc.right - dx;
        int py = rc.top + (rc.bottom - rc.top > 40 ? 15 : (rc.bottom - rc.top) / 2);
        DWORD_PTR res = 0;
        SendMessageTimeoutW(hwnd, WM_NCHITTEST, 0, MAKELPARAM(px, py), SMTO_NORMAL, 500, &res);
        if (static_cast<LRESULT>(res) == HTCLOSE) {
            x = px;
            y = py;
            located = true;
            break;
        }
    }
    if (!located) {
        Out(L"clickclose => titik HTCLOSE tidak terdeteksi (tidak ada klik dikirim)\n");
        return 1;
    }
    Out(L"clickclose: HTCLOSE terdeteksi di (%d,%d)\n", x, y);
    if (!doClick) return 0;
    LPARAM pos = MAKELPARAM(x, y);
    SendMessageTimeoutW(hwnd, WM_NCLBUTTONDOWN, HTCLOSE, pos, SMTO_NORMAL, 500, nullptr);
    Sleep(80);
    SendMessageTimeoutW(hwnd, WM_NCLBUTTONUP, HTCLOSE, pos, SMTO_NORMAL, 500, nullptr);
    Sleep(500);
    bool alive = IsWindow(hwnd) != 0;
    Out(L"klik X => jendela %ls\n", alive ? L"TETAP HIDUP (klik ditolak)" : L"MATI (klik menutup)");
    return alive ? 0 : 4;
}

int wmain(int argc, wchar_t** argv) {
    if (argc < 2) {
        Out(L"pakai: probe list | state <judul> | show <judul> | hide <judul> | gray <judul> | restore <judul> | tryclose <judul>\n");
        return 2;
    }
    std::wstring cmd = argv[1];
    if (cmd == L"list") {
        for (const Win& w : AllTopWindows()) {
            Readout r = ReadClose(w.hwnd);
            Out(L"%p %-22ls %-40ls %ls\n", (void*)w.hwnd, w.proc.c_str(), w.title.c_str(),
                Describe(r).c_str());
        }
        return 0;
    }
    if (argc < 3) {
        Out(L"butuh nama jendela\n");
        return 2;
    }
    HWND hwnd = Resolve(argv[2]);
    if (!hwnd) {
        Out(L"tidak ada jendela yang cocok dengan \"%ls\"\n", argv[2]);
        return 1;
    }
    Readout before = ReadClose(hwnd);
    Dump(L"sebelum", hwnd);
    if (cmd == L"state") return 0;
    if (cmd == L"show") {
        ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
        Sleep(500);
        RECT rc{};
        GetWindowRect(hwnd, &rc);
        Out(L"show => posisi (%ld,%ld)-(%ld,%ld) ukuran %ldx%ld\n", rc.left, rc.top, rc.right,
            rc.bottom, rc.right - rc.left, rc.bottom - rc.top);
        return 0;
    }
    if (cmd == L"tryclose") {
        bool alive = TryClose(hwnd);
        Out(L"tryclose => jendela %ls\n", alive ? L"TETAP HIDUP (close ditolak)"
                                                 : L"MATI (close berhasil)");
        return alive ? 0 : 4;
    }
    if (cmd == L"clickclose") return ClickClose(hwnd, true);
    if (cmd == L"hitclose") return ClickClose(hwnd, false);

    bool ok = false;
    if (cmd == L"hide") ok = RemoveCloseButton(hwnd, true);
    else if (cmd == L"gray") ok = RemoveCloseButton(hwnd, false);
    else if (cmd == L"restore") ok = RestoreCloseButton(hwnd);
    else if (cmd == L"nostyle") ok = SetSysMenuStyle(hwnd, false);
    else if (cmd == L"yesstyle") ok = SetSysMenuStyle(hwnd, true);
    else {
        Out(L"perintah tidak dikenal: %ls\n", cmd.c_str());
        return 2;
    }
    Readout after = ReadClose(hwnd);
    Out(L"aksi %ls => %ls (err=%ls)\n", cmd.c_str(), ok ? L"OK" : L"GAGAL",
        LastErrorText().c_str());
    Dump(L"sesudah", hwnd);
    Out(L"kesimpulan: %ls -> %ls\n", StateName(before.state), StateName(after.state));
    return ok ? 0 : 3;
}
