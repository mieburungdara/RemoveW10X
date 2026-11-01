// drive.cpp - harness verifikasi UI (dev tool).
// Memilih satu jendela di combobox aplikasi, menekan tombol, lalu menDump isi
// log supaya jalur klik -> aksi -> laporan bisa diuji tanpa mouse.
#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "winfind.h"

static void Out(const wchar_t* fmt, ...) {
    wchar_t buf[8192];
    va_list ap;
    va_start(ap, fmt);
    int n = vswprintf(buf, 8192, fmt, ap);
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

struct Kids {
    HWND combo = nullptr;
    HWND edit = nullptr;
    std::vector<HWND> buttons;
};

static BOOL CALLBACK KidProc(HWND h, LPARAM lp) {
    auto* k = reinterpret_cast<Kids*>(lp);
    wchar_t cls[64]{};
    GetClassNameW(h, cls, 64);
    if (wcscmp(cls, L"ComboBox") == 0) k->combo = h;
    else if (wcscmp(cls, L"Edit") == 0) k->edit = h;
    else if (wcscmp(cls, L"Button") == 0) k->buttons.push_back(h);
    return TRUE;
}

static std::wstring TextOf(HWND h) {
    int n = GetWindowTextLengthW(h);
    std::wstring t(n + 1, L'\0');
    GetWindowTextW(h, t.data(), n + 1);
    t.resize(n);
    return t;
}

int wmain(int argc, wchar_t** argv) {
    if (argc < 3) {
        Out(L"pakai: drive <judulApp> <pilihDiCombo> [teksTombol]\n");
        return 2;
    }
    HWND app = Resolve(argv[1]);
    if (!app) {
        Out(L"aplikasi \"%ls\" tidak ditemukan\n", argv[1]);
        return 1;
    }
    Kids k;
    EnumChildWindows(app, KidProc, reinterpret_cast<LPARAM>(&k));
    if (!k.combo) {
        Out(L"combobox tidak ditemukan\n");
        return 1;
    }
    int count = static_cast<int>(SendMessageW(k.combo, CB_GETCOUNT, 0, 0));
    auto itemAt = [&](int i) {
        int len = static_cast<int>(SendMessageW(k.combo, CB_GETLBTEXTLEN, i, 0));
        std::wstring t(len + 1, L'\0');
        SendMessageW(k.combo, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(t.data()));
        t.resize(len);
        return t;
    };
    if (std::wstring(argv[2]) == L"-list") {
        for (int i = 0; i < count; ++i) Out(L"combo[%d] = %ls\n", i, itemAt(i).c_str());
        return 0;
    }
    std::wstring needle = ToLower(argv[2]);
    int pick = -1;
    for (int i = 0; i < count; ++i) {
        std::wstring t = itemAt(i);
        if (ToLower(t).find(needle) != std::wstring::npos) {
            pick = i;
            Out(L"combo[%d] = %ls\n", i, t.c_str());
            break;
        }
    }
    if (pick < 0) {
        Out(L"TIDAK ADA item combo yang mengandung \"%ls\" (dari %d item)\n", argv[2], count);
        return 1;
    }
    SendMessageW(k.combo, CB_SETCURSEL, pick, 0);
    Out(L"dipilih index %d\n", pick);

    if (argc >= 4) {
        std::wstring bn = ToLower(argv[3]);
        bool clicked = false;
        if (!bn.empty() && bn[0] == L'#') {
            // Klik lewat control id: dipakai tombol yang teksnya gambar bitmap.
            int want = _wtoi(argv[3] + 1);
            for (HWND b : k.buttons) {
                if (static_cast<int>(GetDlgCtrlID(b)) != want) continue;
                Out(L"klik control id=%d\n", want);
                SendMessageW(app, WM_COMMAND, MAKEWPARAM(want, BN_CLICKED),
                             reinterpret_cast<LPARAM>(b));
                Sleep(400);
                clicked = true;
                break;
            }
        }
        if (!clicked) {
            for (HWND b : k.buttons) {
                std::wstring t = ToLower(TextOf(b));
                if (t.find(bn) == std::wstring::npos) continue;
                int id = static_cast<int>(GetDlgCtrlID(b));
                Out(L"klik tombol \"%ls\" (id=%d)\n", TextOf(b).c_str(), id);
                // BN_CLICKED langsung ke parent: BM_CLICK pada default button bisa
                // mengirim notifikasi dua kali.
                SendMessageW(app, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED),
                             reinterpret_cast<LPARAM>(b));
                Sleep(400);
                clicked = true;
                break;
            }
        }
        if (!clicked) Out(L"TOMBOL \"%ls\" tidak ditemukan\n", argv[3]);
    }
    if (k.edit) {
        int len = static_cast<int>(SendMessageW(k.edit, WM_GETTEXTLENGTH, 0, 0));
        std::wstring t(len + 1, L'\0');
        SendMessageW(k.edit, WM_GETTEXT, len + 1, reinterpret_cast<LPARAM>(t.data()));
        Out(L"--- ISI LOG ---\n%ls\n--- AKHIR LOG ---\n", t.c_str());
    }
    return 0;
}
