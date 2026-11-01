#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <commctrl.h>

#include <string>
#include <vector>

#include "closebtn.h"
#include "winfind.h"

enum : int {
    IDC_COMBO = 101,
    IDC_LOG = 102,
    IDC_REFRESH = 103,
    IDC_HIDE_X = 104,
    IDC_HIDE_CAPTION = 105,
    IDC_RESTORE = 106,
};

static HWND g_combo = nullptr;
static HWND g_log = nullptr;
static std::vector<HWND> g_items;
static DWORD g_ownPid = 0;

static void Log(const std::wstring& text) {
    if (!g_log) return;
    if (SendMessageW(g_log, WM_GETTEXTLENGTH, 0, 0) > 0) {
        SendMessageW(g_log, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
        SendMessageW(g_log, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(L"\r\n"));
    }
    SendMessageW(g_log, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
    SendMessageW(g_log, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text.c_str()));
    SendMessageW(g_log, EM_SCROLLCARET, 0, 0);
}

static std::wstring TargetLabel(HWND hwnd) {
    return WindowTitle(hwnd) + L"  (" + ProcName(hwnd) + L")";
}

static void RefreshList(HWND keep) {
    SendMessageW(g_combo, CB_RESETCONTENT, 0, 0);
    g_items.clear();
    for (const Win& w : AllTopWindows()) {
        DWORD pid = 0;
        GetWindowThreadProcessId(w.hwnd, &pid);
        if (pid == g_ownPid) continue;
        std::wstring label = w.title + L"   |   " + w.proc;
        LRESULT idx = SendMessageW(g_combo, CB_ADDSTRING, 0,
                                   reinterpret_cast<LPARAM>(label.c_str()));
        if (idx == CB_ERR) continue;
        g_items.push_back(w.hwnd);
        if (w.hwnd == keep) SendMessageW(g_combo, CB_SETCURSEL, idx, 0);
    }
    if (SendMessageW(g_combo, CB_GETCURSEL, 0, 0) == CB_ERR && !g_items.empty()) {
        SendMessageW(g_combo, CB_SETCURSEL, 0, 0);
    }
    Log(L"Daftar dimuat: " + std::to_wstring(g_items.size()) +
                         L" jendela milik proses lain.");
}

static HWND Selected() {
    LRESULT i = SendMessageW(g_combo, CB_GETCURSEL, 0, 0);
    if (i == CB_ERR || i < 0 || static_cast<size_t>(i) >= g_items.size()) {
        Log(L"Pilih satu jendela dulu.");
        return nullptr;
    }
    HWND hwnd = g_items[static_cast<size_t>(i)];
    if (!IsWindow(hwnd)) {
        Log(L"Jendela itu sudah tertutup sendiri. Muat ulang daftar.");
        return nullptr;
    }
    return hwnd;
}

static void ReportAfter(HWND hwnd, const MenuProbe& before, bool applied,
                        const wchar_t* action) {
    Log(L"target: " + TargetLabel(hwnd));
    Log(L"sebelum : " + DescribeMenu(before));
    if (!applied) {
        Log(std::wstring(action) + L" => GAGAL (err=" + LastErrorText() + L")");
        return;
    }
    Log(std::wstring(action) + L" => diterapkan");
    Log(L"sesudah : " + DescribeMenu(ProbeClose(hwnd)));
    Log(
              L"Lihat judul jendela target: X harusnya jadi abu-abu/hilang. Status di atas dibaca "
              L"dari salinan menu di proses ini, jadi bukti akhirnya tetap tampilan target.");
}

static void DoHideX() {
    HWND hwnd = Selected();
    if (!hwnd) return;
    MenuProbe before = ProbeClose(hwnd);
    if (before.hasStyle && !before.readable) {
        // Terukur: proses baru sering mendapat handle menu yang tidak bisa dipakai.
        // Reset (GetSystemMenu TRUE) membuat salinan yang sah di proses ini, dan
        // sekaligus mengembalikan menu ke default.
        Log(L"target: " + TargetLabel(hwnd));
        Log(L"Handle menu belum bisa dipakai dari proses ini (err " +
            std::to_wstring(before.err) + L") - reset menu dulu.");
        RestoreCloseButton(hwnd);
        before = ProbeClose(hwnd);
        Log(L"sesudah reset: " + DescribeMenu(before));
        if (!before.readable) {
            Log(L"Masih tidak bisa. Untuk jendela ini pakai 'Hilangkan semua tombol caption'.");
            return;
        }
    }
    if (before.state == CloseState::Removed) {
        Log(L"target: " + TargetLabel(hwnd));
        Log(L"Item SC_CLOSE sudah tidak ada, tidak ada yang diubah.");
        return;
    }
    ReportAfter(hwnd, before, RemoveCloseButton(hwnd, true), L"hapus item SC_CLOSE");
}

static void DoHideCaption() {
    HWND hwnd = Selected();
    if (!hwnd) return;
    bool ok = SetSysMenuStyle(hwnd, false);
    Log(L"target: " + TargetLabel(hwnd));
    if (!ok) {
        Log(L"WS_SYSMENU -> OFF : GAGAL (err=" + LastErrorText() +
                             L"). Jendela milik proses elevated/admin tidak bisa diubah dari "
                             L"proses non-admin.");
        return;
    }
    Log(L"WS_SYSMENU -> OFF : berhasil. Minimize, maximize, dan close hilang "
                     L"bersamaan.");
    Log(L"Setelah ini status menu tidak terbaca lagi: Windows meng-reset handle "
                     L"menu di proses ini saat style jendela berubah.");
}

static void DoRestore() {
    HWND hwnd = Selected();
    if (!hwnd) return;
    MenuProbe before = ProbeClose(hwnd);
    bool styleBack = SetSysMenuStyle(hwnd, true);
    std::wstring styleErr = styleBack ? L"" : LastErrorText();
    RestoreCloseButton(hwnd);
    MenuProbe after = ProbeClose(hwnd);
    Log(L"target: " + TargetLabel(hwnd));
    Log(L"WS_SYSMENU -> ON : " + std::wstring(styleBack ? L"berhasil" : L"GAGAL err=" + styleErr));
    if (after.state == CloseState::Enabled) {
        Log(L"tombol close   : AKTIF lagi, item=" + std::to_wstring(after.items));
    } else {
        Log(std::wstring(L"tombol close   : masih ") + StateName(after.state) +
            L", item=" + std::to_wstring(after.items) +
            L" (reset menu dari proses ini tidak diterima Windows)");
    }
    Log(std::wstring(L"sebelum=") + StateName(before.state) + L"  sesudah=" +
        StateName(after.state));
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            HINSTANCE inst = reinterpret_cast<LPCREATESTRUCTW>(lp)->hInstance;
            NONCLIENTMETRICSW ncm{};
            ncm.cbSize = sizeof(ncm);
            SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
            HFONT font = CreateFontIndirectW(&ncm.lfMessageFont);

            RECT rc{};
            GetClientRect(hwnd, &rc);
            const int m = 12;
            const int bw = rc.right - 2 * m;

            auto make = [&](const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y,
                            int w, int h, int id, DWORD ex = 0) {
                HWND c = CreateWindowExW(ex, cls, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h,
                                         hwnd, reinterpret_cast<HMENU>(id), inst, nullptr);
                SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
                return c;
            };

            make(L"STATIC", L"Jendela target (pilih satu, lalu klik aksi):", 0, m, m, bw, 18, 0);

            g_combo = make(L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, m,
                           m + 20, bw, 340, IDC_COMBO);

            struct Btn {
                int id;
                const wchar_t* text;
                int x;
                int w;
                bool def;
            };
            const Btn btns[] = {
                {IDC_REFRESH, L"Muat ulang", m, 90, false},
                {IDC_HIDE_X, L"Hilangkan tombol X saja", m + 96, 190, true},
                {IDC_HIDE_CAPTION, L"Hilangkan semua tombol caption", m + 292, 230, false},
                {IDC_RESTORE, L"Kembalikan", m + 528, 90, false},
            };
            for (const Btn& b : btns) {
                make(L"BUTTON", b.text,
                     WS_TABSTOP | (b.def ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON), b.x, m + 50, b.w,
                     26, b.id);
            }

            make(L"STATIC", L"Log aksi:", 0, m, m + 84, bw, 18, 0);
            g_log = make(L"EDIT", L"",
                         WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, m, m + 102, bw,
                         rc.bottom - (m + 102) - m, IDC_LOG, WS_EX_CLIENTEDGE);

            Log(L"Dua mode, keduanya bisa dibalik:");
            Log(L"1. Hilangkan tombol X saja = buang item SC_CLOSE dari system menu. "
                            L"Hanya X yang kena, tapi tidak berlaku untuk semua jendela.");
            Log(L"2. Hilangkan semua tombol caption = matikan WS_SYSMENU. Berlaku "
                            L"untuk semua jendela non-admin, tapi minimize/maximize ikut hilang.");
            Log(L"Batas penting: ini menghentikan KLIK pada tombol. Alt+F4, tutup "
                            L"dari taskbar, dan app.close() target tetap bisa menutup jendela.");
            RefreshList(nullptr);
            return 0;
        }
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDC_REFRESH: {
                    HWND keep = g_combo ? Selected() : nullptr;
                    RefreshList(keep);
                    return 0;
                }
                case IDC_HIDE_X:
                    DoHideX();
                    return 0;
                case IDC_HIDE_CAPTION:
                    DoHideCaption();
                    return 0;
                case IDC_RESTORE:
                    DoRestore();
                    return 0;
            }
            break;
        case WM_CTLCOLORSTATIC: {
            HDC dc = reinterpret_cast<HDC>(wp);
            if (reinterpret_cast<HWND>(lp) == g_log) {
                SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
                SetBkColor(dc, GetSysColor(COLOR_WINDOW));
                return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
            }
            SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
            SetBkColor(dc, GetSysColor(COLOR_BTNFACE));
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int show) {
    g_ownPid = GetCurrentProcessId();
    SetProcessDPIAware();
    INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    wc.lpszClassName = L"CloseButtonToolMain";
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"Hilangkan tombol close - Win32",
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                CW_USEDEFAULT, CW_USEDEFAULT, 720, 430, nullptr, nullptr, inst,
                                nullptr);
    ShowWindow(hwnd, show ? show : SW_SHOWDEFAULT);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return static_cast<int>(msg.wParam);
}
