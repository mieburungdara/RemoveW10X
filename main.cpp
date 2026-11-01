#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <commctrl.h>

#include <initializer_list>
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
    IDC_LANG_ID = 107,
    IDC_LANG_EN = 108,
};

// Semua teks UI. Menambah bahasa = menambah satu struct L10N, bukan if/else
// tersebar di kode.
struct L10N {
    const wchar_t* title;
    const wchar_t* pickLabel;
    const wchar_t* logLabel;
    const wchar_t* btnRefresh;
    const wchar_t* btnHideX;
    const wchar_t* btnHideCaption;
    const wchar_t* btnRestore;
    const wchar_t* ok;
    const wchar_t* fail;
    // Status menu.
    const wchar_t* stNoMenu;
    const wchar_t* stRemoved;
    const wchar_t* stDisabled;
    const wchar_t* stEnabled;
    const wchar_t* unreadableNote;
    // Log.
    const wchar_t* introHead;
    const wchar_t* introMode1;
    const wchar_t* introMode2;
    const wchar_t* introLimit;
    const wchar_t* loaded;
    const wchar_t* pickFirst;
    const wchar_t* closedItself;
    const wchar_t* targetLine;
    const wchar_t* beforeLine;
    const wchar_t* afterLine;
    const wchar_t* appliedLine;
    const wchar_t* failedLine;
    const wchar_t* actionRemoveSc;
    const wchar_t* seeCaption;
    const wchar_t* resetFirst;
    const wchar_t* afterReset;
    const wchar_t* stillNope;
    const wchar_t* alreadyGone;
    const wchar_t* styleOffOk;
    const wchar_t* styleMenuNote;
    const wchar_t* styleOffFail;
    const wchar_t* styleOnLine;
    const wchar_t* closeEnabled;
    const wchar_t* closeStill;
    const wchar_t* stateChange;
    const wchar_t* switched;
};

static const L10N kEn = {
    .title = L"Remove close button - Win32",
    .pickLabel = L"Target window (pick one, then click an action):",
    .logLabel = L"Action log:",
    .btnRefresh = L"Reload",
    .btnHideX = L"Remove X button only",
    .btnHideCaption = L"Hide all caption buttons",
    .btnRestore = L"Restore",
    .ok = L"done",
    .fail = L"FAILED",
    .stNoMenu = L"has no system menu",
    .stRemoved = L"GONE (item deleted)",
    .stDisabled = L"GREYED (disabled)",
    .stEnabled = L"ENABLED",
    .unreadableNote = L" - not readable from this process (menu handle belongs to the target)",
    .introHead = L"Two modes, both reversible:",
    .introMode1 = L"1. Remove X button only = delete the SC_CLOSE item from the window's system "
                  L"menu. Only X is affected, but it does not work on every window.",
    .introMode2 = L"2. Hide all caption buttons = clear WS_SYSMENU. Works on every non-admin "
                  L"window, but minimize and maximize disappear too.",
    .introLimit = L"Important limit: this only stops the CLICK on the button. Alt+F4, the taskbar "
                  L"close, and the target's own app.close() still close the window.",
    .loaded = L"List loaded: {0} windows owned by other processes.",
    .pickFirst = L"Pick a window first.",
    .closedItself = L"That window already closed itself. Reload the list.",
    .targetLine = L"target: {0}",
    .beforeLine = L"before  : {0}",
    .afterLine = L"after   : {0}",
    .appliedLine = L"{0} => applied",
    .failedLine = L"{0} => FAILED (err={1})",
    .actionRemoveSc = L"remove SC_CLOSE item",
    .seeCaption = L"Check the target's title bar: the X should be greyed/gone. The status above "
                  L"comes from this process's copy of the menu, so the target's caption is the "
                  L"real proof.",
    .resetFirst = L"Menu handle not usable from this process yet (err {0}) - resetting the menu.",
    .afterReset = L"after reset: {0}",
    .stillNope = L"Still not possible. For this window use 'Hide all caption buttons'.",
    .alreadyGone = L"The SC_CLOSE item is already gone; nothing changed.",
    .styleOffOk = L"WS_SYSMENU -> OFF: done. Minimize, maximize and close disappear together.",
    .styleMenuNote = L"After this the menu status is unreadable: Windows resets this process's "
                     L"menu handle when the window style changes.",
    .styleOffFail = L"WS_SYSMENU -> OFF: FAILED (err={0}). Windows refuses changes to elevated "
                    L"windows from a non-admin process.",
    .styleOnLine = L"WS_SYSMENU -> ON : {0}",
    .closeEnabled = L"close button  : ENABLED again, items={0}",
    .closeStill = L"close button  : still {0}, items={1} (Windows refused the menu reset from "
                  L"this process)",
    .stateChange = L"before={0}  after={1}",
    .switched = L"Language switched to English.",
};

static const L10N kId = {
    .title = L"Hilangkan tombol close - Win32",
    .pickLabel = L"Jendela target (pilih satu, lalu klik aksi):",
    .logLabel = L"Log aksi:",
    .btnRefresh = L"Muat ulang",
    .btnHideX = L"Hilangkan tombol X saja",
    .btnHideCaption = L"Hilangkan semua tombol caption",
    .btnRestore = L"Kembalikan",
    .ok = L"berhasil",
    .fail = L"GAGAL",
    .stNoMenu = L"tidak punya system menu",
    .stRemoved = L"HILANG (item dihapus)",
    .stDisabled = L"ABU-ABU (disabled)",
    .stEnabled = L"AKTIF",
    .unreadableNote = L" - TIDAK TERBACA dari proses ini (handle menu milik proses target)",
    .introHead = L"Dua mode, keduanya bisa dibalik:",
    .introMode1 = L"1. Hilangkan tombol X saja = buang item SC_CLOSE dari system menu. Hanya X "
                  L"yang kena, tapi tidak berlaku untuk semua jendela.",
    .introMode2 = L"2. Hilangkan semua tombol caption = matikan WS_SYSMENU. Berlaku untuk semua "
                  L"jendela non-admin, tapi minimize/maximize ikut hilang.",
    .introLimit = L"Batas penting: ini menghentikan KLIK pada tombol. Alt+F4, tutup dari taskbar, "
                  L"dan app.close() target tetap bisa menutup jendela.",
    .loaded = L"Daftar dimuat: {0} jendela milik proses lain.",
    .pickFirst = L"Pilih satu jendela dulu.",
    .closedItself = L"Jendela itu sudah tertutup sendiri. Muat ulang daftar.",
    .targetLine = L"target: {0}",
    .beforeLine = L"sebelum : {0}",
    .afterLine = L"sesudah : {0}",
    .appliedLine = L"{0} => diterapkan",
    .failedLine = L"{0} => GAGAL (err={1})",
    .actionRemoveSc = L"hapus item SC_CLOSE",
    .seeCaption = L"Lihat judul jendela target: X harusnya jadi abu-abu/hilang. Status di atas "
                  L"dibaca dari salinan menu di proses ini, jadi bukti akhirnya tetap tampilan "
                  L"target.",
    .resetFirst = L"Handle menu belum bisa dipakai dari proses ini (err {0}) - reset menu dulu.",
    .afterReset = L"sesudah reset: {0}",
    .stillNope = L"Masih tidak bisa. Untuk jendela ini pakai 'Hilangkan semua tombol caption'.",
    .alreadyGone = L"Item SC_CLOSE sudah tidak ada, tidak ada yang diubah.",
    .styleOffOk = L"WS_SYSMENU -> OFF : berhasil. Minimize, maximize, dan close hilang "
                  L"bersamaan.",
    .styleMenuNote = L"Setelah ini status menu tidak terbaca lagi: Windows meng-reset handle menu "
                     L"di proses ini saat style jendela berubah.",
    .styleOffFail = L"WS_SYSMENU -> OFF : GAGAL (err={0}). Jendela milik proses elevated/admin "
                    L"tidak bisa diubah dari proses non-admin.",
    .styleOnLine = L"WS_SYSMENU -> ON : {0}",
    .closeEnabled = L"tombol close   : AKTIF lagi, item={0}",
    .closeStill = L"tombol close   : masih {0}, item={1} (reset menu dari proses ini tidak "
                  L"diterima Windows)",
    .stateChange = L"sebelum={0}  sesudah={1}",
    .switched = L"Bahasa diubah ke Indonesia.",
};

static const L10N* S = &kEn;

static HWND g_combo = nullptr;
static HWND g_log = nullptr;
static HWND g_statPick = nullptr;
static HWND g_statLog = nullptr;
static HWND g_btnRefresh = nullptr;
static HWND g_btnHideX = nullptr;
static HWND g_btnCaption = nullptr;
static HWND g_btnRestore = nullptr;
static HWND g_btnId = nullptr;
static HWND g_btnEn = nullptr;
static HFONT g_font = nullptr;
static HBITMAP g_flagId = nullptr;
static HBITMAP g_flagEn = nullptr;
static HICON g_iconBig = nullptr;
static HICON g_iconSmall = nullptr;
static std::vector<HWND> g_items;
static DWORD g_ownPid = 0;

static void RefreshList();

static std::wstring Fmt(const wchar_t* tpl, std::initializer_list<std::wstring> args) {
    std::wstring out = tpl;
    int i = 0;
    for (const std::wstring& a : args) {
        std::wstring ph = L"{" + std::to_wstring(i++) + L"}";
        size_t at = 0;
        while ((at = out.find(ph, at)) != std::wstring::npos) {
            out.replace(at, ph.size(), a);
            at += a.size();
        }
    }
    return out;
}

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

static const wchar_t* StateText(CloseState st) {
    switch (st) {
        case CloseState::NoSystemMenu: return S->stNoMenu;
        case CloseState::Removed:      return S->stRemoved;
        case CloseState::Disabled:     return S->stDisabled;
        case CloseState::Enabled:      return S->stEnabled;
    }
    return S->stEnabled;
}

static std::wstring DescribeLocal(const MenuProbe& p) {
    std::wstring s = StateText(p.state);
    if (!p.readable && p.hasStyle) s += S->unreadableNote;
    s += L", item=" + std::to_wstring(p.items) + L", err=" + std::to_wstring(p.err);
    return s;
}

static std::wstring TargetLabel(HWND hwnd) {
    return WindowTitle(hwnd) + L"  (" + ProcName(hwnd) + L")";
}

// ---- Bendera: digambar dengan GDI, bukan emoji ----
// Segoe UI Emoji di Win10 tidak merender bendera sebagai satu glif lewat GDI
// (hasilnya huruf regional indicator), jadi bitmap dibuat sendiri.
static void DrawFlag(HDC dc, int x, int y, int w, int h, bool english) {
    RECT f{x, y, x + w, y + h};
    HRGN clip = CreateRectRgnIndirect(&f);
    SelectClipRgn(dc, clip);
    DeleteObject(clip);

    auto fill = [&](COLORREF c, RECT r) {
        HBRUSH b = CreateSolidBrush(c);
        FillRect(dc, &r, b);
        DeleteObject(b);
    };
    const COLORREF red = RGB(210, 16, 32);
    const COLORREF blue = RGB(10, 32, 96);

    if (!english) {
        fill(red, RECT{x, y, x + w, y + h / 2});
        fill(RGB(255, 255, 255), RECT{x, y + h / 2, x + w, y + h});
    } else {
        fill(blue, f);
        HPEN white = CreatePen(PS_SOLID, std::max(2, h / 4), RGB(255, 255, 255));
        HPEN old = static_cast<HPEN>(SelectObject(dc, white));
        MoveToEx(dc, x, y, nullptr);
        LineTo(dc, x + w, y + h);
        MoveToEx(dc, x + w, y, nullptr);
        LineTo(dc, x, y + h);
        SelectObject(dc, old);
        DeleteObject(white);

        HPEN redpen = CreatePen(PS_SOLID, 1, red);
        old = static_cast<HPEN>(SelectObject(dc, redpen));
        MoveToEx(dc, x, y, nullptr);
        LineTo(dc, x + w, y + h);
        MoveToEx(dc, x + w, y, nullptr);
        LineTo(dc, x, y + h);
        SelectObject(dc, old);
        DeleteObject(redpen);

        int cw = std::max(3, h / 3);
        int rw = std::max(2, h / 5);
        int cx = x + w / 2, cy = y + h / 2;
        fill(RGB(255, 255, 255), RECT{cx - cw / 2, y, cx - cw / 2 + cw, y + h});
        fill(RGB(255, 255, 255), RECT{x, cy - cw / 2, x + w, cy - cw / 2 + cw});
        fill(red, RECT{cx - rw / 2, y, cx - rw / 2 + rw, y + h});
        fill(red, RECT{x, cy - rw / 2, x + w, cy - rw / 2 + rw});
    }
    SelectClipRgn(dc, nullptr);
    HPEN frame = CreatePen(PS_SOLID, 1, RGB(70, 70, 70));
    HPEN oldpen = static_cast<HPEN>(SelectObject(dc, frame));
    HBRUSH none = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
    HBRUSH oldbr = static_cast<HBRUSH>(SelectObject(dc, none));
    Rectangle(dc, x, y, x + w + 1, y + h + 1);
    SelectObject(dc, oldpen);
    SelectObject(dc, oldbr);
    DeleteObject(frame);
}

static HBITMAP MakeFlagButton(HDC ref, const wchar_t* label, bool english, bool active) {
    const int W = 54, H = 22, FW = 20, FH = 14;
    HBITMAP bmp = CreateCompatibleBitmap(ref, W, H);
    HDC dc = CreateCompatibleDC(ref);
    HBITMAP old = static_cast<HBITMAP>(SelectObject(dc, bmp));

    COLORREF bg = active ? GetSysColor(COLOR_HIGHLIGHT) : GetSysColor(COLOR_BTNFACE);
    COLORREF fg = active ? RGB(255, 255, 255) : GetSysColor(COLOR_WINDOWTEXT);
    RECT full{0, 0, W, H};
    HBRUSH b = CreateSolidBrush(bg);
    FillRect(dc, &full, b);
    DeleteObject(b);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, fg);
    HFONT oldf = static_cast<HFONT>(SelectObject(dc, g_font));
    SIZE sz{};
    GetTextExtentPoint32W(dc, label, lstrlenW(label), &sz);
    TextOutW(dc, FW + 8, (H - sz.cy) / 2, label, lstrlenW(label));
    SelectObject(dc, oldf);

    DrawFlag(dc, 4, (H - FH) / 2, FW, FH, english);

    if (active) {
        HPEN p = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        HPEN op = static_cast<HPEN>(SelectObject(dc, p));
        HBRUSH nb = static_cast<HBRUSH>(SelectObject(dc, GetStockObject(NULL_BRUSH)));
        Rectangle(dc, 0, 0, W, H);
        SelectObject(dc, op);
        SelectObject(dc, nb);
        DeleteObject(p);
    }
    SelectObject(dc, old);
    DeleteDC(dc);
    return bmp;
}

static void RefreshLangButtons() {
    bool idActive = (S == &kId);
    HDC ref = GetDC(nullptr);
    if (g_flagId) DeleteObject(g_flagId);
    if (g_flagEn) DeleteObject(g_flagEn);
    g_flagId = MakeFlagButton(ref, L"ID", false, idActive);
    g_flagEn = MakeFlagButton(ref, L"EN", true, !idActive);
    ReleaseDC(nullptr, ref);
    SendMessageW(g_btnId, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(g_flagId));
    SendMessageW(g_btnEn, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(g_flagEn));
}

// Tombol aksi diberi lebar sesuai teksnya supaya bahasa mana pun tidak terpotong.
static void LayoutActions(HWND hwnd) {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    const int m = 12, gap = 6, h = 26;
    const HWND btns[] = {g_btnRefresh, g_btnHideX, g_btnCaption, g_btnRestore};
    const wchar_t* texts[] = {S->btnRefresh, S->btnHideX, S->btnHideCaption, S->btnRestore};
    HDC dc = GetDC(hwnd);
    HFONT old = static_cast<HFONT>(SelectObject(dc, g_font));
    int x = m;
    for (int i = 0; i < 4; ++i) {
        SIZE sz{};
        GetTextExtentPoint32W(dc, texts[i], lstrlenW(texts[i]), &sz);
        int w = sz.cx + 22;
        MoveWindow(btns[i], x, m + 50, w, h, TRUE);
        x += w + gap;
    }
    SelectObject(dc, old);
    ReleaseDC(hwnd, dc);
}

static void ApplyLanguage(HWND hwnd, const L10N* next) {
    S = next;
    SetWindowTextW(hwnd, S->title);
    SetWindowTextW(g_statPick, S->pickLabel);
    SetWindowTextW(g_statLog, S->logLabel);
    SetWindowTextW(g_btnRefresh, S->btnRefresh);
    SetWindowTextW(g_btnHideX, S->btnHideX);
    SetWindowTextW(g_btnCaption, S->btnHideCaption);
    SetWindowTextW(g_btnRestore, S->btnRestore);
    RefreshLangButtons();
    LayoutActions(hwnd);
    SendMessageW(g_log, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L""));
    Log(S->introHead);
    Log(S->introMode1);
    Log(S->introMode2);
    Log(S->introLimit);
    Log(S->switched);
    RefreshList();
}

static void RefreshList() {
    SendMessageW(g_combo, CB_RESETCONTENT, 0, 0);
    HWND keep = nullptr;
    LRESULT cur = SendMessageW(g_combo, CB_GETCURSEL, 0, 0);
    if (cur != CB_ERR && static_cast<size_t>(cur) < g_items.size()) keep = g_items[cur];
    g_items.clear();
    for (const Win& w : AllTopWindows()) {
        DWORD pid = 0;
        GetWindowThreadProcessId(w.hwnd, &pid);
        if (pid == g_ownPid) continue;
        std::wstring label = w.title + L"   |   " + w.proc;
        LRESULT idx =
            SendMessageW(g_combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        if (idx == CB_ERR) continue;
        g_items.push_back(w.hwnd);
        if (w.hwnd == keep) SendMessageW(g_combo, CB_SETCURSEL, idx, 0);
    }
    if (SendMessageW(g_combo, CB_GETCURSEL, 0, 0) == CB_ERR && !g_items.empty()) {
        SendMessageW(g_combo, CB_SETCURSEL, 0, 0);
    }
    Log(Fmt(S->loaded, {std::to_wstring(g_items.size())}));
}

static HWND Selected() {
    LRESULT i = SendMessageW(g_combo, CB_GETCURSEL, 0, 0);
    if (i == CB_ERR || i < 0 || static_cast<size_t>(i) >= g_items.size()) {
        Log(S->pickFirst);
        return nullptr;
    }
    HWND hwnd = g_items[static_cast<size_t>(i)];
    if (!IsWindow(hwnd)) {
        Log(S->closedItself);
        return nullptr;
    }
    return hwnd;
}

static void ReportAfter(HWND hwnd, const MenuProbe& before, bool applied, const wchar_t* action) {
    Log(Fmt(S->targetLine, {TargetLabel(hwnd)}));
    Log(Fmt(S->beforeLine, {DescribeLocal(before)}));
    if (!applied) {
        Log(Fmt(S->failedLine, {action, LastErrorText()}));
        return;
    }
    Log(Fmt(S->appliedLine, {action}));
    Log(Fmt(S->afterLine, {DescribeLocal(ProbeClose(hwnd))}));
    Log(S->seeCaption);
}

static void DoHideX() {
    HWND hwnd = Selected();
    if (!hwnd) return;
    MenuProbe before = ProbeClose(hwnd);
    if (before.hasStyle && !before.readable) {
        // Terukur: proses baru sering mendapat handle menu yang tidak bisa dipakai.
        // Reset (GetSystemMenu TRUE) membuat salinan yang sah di proses ini, dan
        // sekaligus mengembalikan menu ke default.
        Log(Fmt(S->targetLine, {TargetLabel(hwnd)}));
        Log(Fmt(S->resetFirst, {std::to_wstring(before.err)}));
        RestoreCloseButton(hwnd);
        before = ProbeClose(hwnd);
        Log(Fmt(S->afterReset, {DescribeLocal(before)}));
        if (!before.readable) {
            Log(S->stillNope);
            return;
        }
    }
    if (before.state == CloseState::Removed) {
        Log(Fmt(S->targetLine, {TargetLabel(hwnd)}));
        Log(S->alreadyGone);
        return;
    }
    ReportAfter(hwnd, before, RemoveCloseButton(hwnd, true), S->actionRemoveSc);
}

static void DoHideCaption() {
    HWND hwnd = Selected();
    if (!hwnd) return;
    bool ok = SetSysMenuStyle(hwnd, false);
    Log(Fmt(S->targetLine, {TargetLabel(hwnd)}));
    if (!ok) {
        Log(Fmt(S->styleOffFail, {LastErrorText()}));
        return;
    }
    Log(S->styleOffOk);
    Log(S->styleMenuNote);
}

static void DoRestore() {
    HWND hwnd = Selected();
    if (!hwnd) return;
    MenuProbe before = ProbeClose(hwnd);
    bool styleBack = SetSysMenuStyle(hwnd, true);
    std::wstring styleErr = styleBack ? L"" : LastErrorText();
    RestoreCloseButton(hwnd);
    MenuProbe after = ProbeClose(hwnd);
    Log(Fmt(S->targetLine, {TargetLabel(hwnd)}));
    std::wstring styleResult =
        styleBack ? std::wstring(S->ok) : std::wstring(S->fail) + L" err=" + styleErr;
    Log(Fmt(S->styleOnLine, {styleResult}));
    if (after.state == CloseState::Enabled) {
        Log(Fmt(S->closeEnabled, {std::to_wstring(after.items)}));
    } else {
        Log(Fmt(S->closeStill, {StateText(after.state), std::to_wstring(after.items)}));
    }
    Log(Fmt(S->stateChange, {StateText(before.state), StateText(after.state)}));
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            HINSTANCE inst = reinterpret_cast<LPCREATESTRUCTW>(lp)->hInstance;
            NONCLIENTMETRICSW ncm{};
            ncm.cbSize = sizeof(ncm);
            SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
            g_font = CreateFontIndirectW(&ncm.lfMessageFont);

            RECT rc{};
            GetClientRect(hwnd, &rc);
            const int m = 12;
            const int flagW = 54, flagGap = 6;
            const int bw = rc.right - 2 * m - 2 * flagW - flagGap - 8;

            auto make = [&](const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y,
                            int w, int h, int id, DWORD ex = 0) {
                HWND c = CreateWindowExW(ex, cls, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h,
                                         hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                         inst, nullptr);
                SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);
                return c;
            };

            g_statPick = make(L"STATIC", L"", 0, m, m, bw, 18, 0);
            g_combo = make(L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, m,
                           m + 20, rc.right - 2 * m, 340, IDC_COMBO);

            g_btnRefresh = make(L"BUTTON", L"", WS_TABSTOP | BS_PUSHBUTTON, m, m + 50, 90, 26,
                                IDC_REFRESH);
            g_btnHideX = make(L"BUTTON", L"", WS_TABSTOP | BS_DEFPUSHBUTTON, m + 96, m + 50, 190,
                              26, IDC_HIDE_X);
            g_btnCaption = make(L"BUTTON", L"", WS_TABSTOP | BS_PUSHBUTTON, m + 292, m + 50, 230,
                                26, IDC_HIDE_CAPTION);
            g_btnRestore = make(L"BUTTON", L"", WS_TABSTOP | BS_PUSHBUTTON, m + 528, m + 50, 90,
                                26, IDC_RESTORE);

            int fx = rc.right - m - flagW;
            g_btnId = make(L"BUTTON", L"", WS_TABSTOP | BS_BITMAP, fx - flagGap - flagW, m - 1,
                           flagW, 22, IDC_LANG_ID);
            g_btnEn = make(L"BUTTON", L"", WS_TABSTOP | BS_BITMAP, fx, m - 1, flagW, 22,
                           IDC_LANG_EN);

            g_statLog = make(L"STATIC", L"", 0, m, m + 84, rc.right - 2 * m, 18, 0);
            g_log = make(L"EDIT", L"", WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, m,
                         m + 102, rc.right - 2 * m, rc.bottom - (m + 102) - m, IDC_LOG,
                         WS_EX_CLIENTEDGE);

            ApplyLanguage(hwnd, S);
            return 0;
        }
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDC_REFRESH:
                    RefreshList();
                    return 0;
                case IDC_HIDE_X:
                    DoHideX();
                    return 0;
                case IDC_HIDE_CAPTION:
                    DoHideCaption();
                    return 0;
                case IDC_RESTORE:
                    DoRestore();
                    return 0;
                case IDC_LANG_ID:
                    ApplyLanguage(hwnd, &kId);
                    return 0;
                case IDC_LANG_EN:
                    ApplyLanguage(hwnd, &kEn);
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
            if (g_iconBig) DestroyIcon(g_iconBig);
            if (g_iconSmall) DestroyIcon(g_iconSmall);
            if (g_flagId) DeleteObject(g_flagId);
            if (g_flagEn) DeleteObject(g_flagEn);
            if (g_font) DeleteObject(g_font);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static const L10N* DefaultLang() {
    return ((GetUserDefaultUILanguage() & 0x3FF) == LANG_INDONESIAN) ? &kId : &kEn;
}

int wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int show) {
    g_ownPid = GetCurrentProcessId();
    S = DefaultLang();
    SetProcessDPIAware();
    INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    wc.lpszClassName = L"RemoveW10XMain";
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"RemoveW10X",
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                CW_USEDEFAULT, CW_USEDEFAULT, 760, 430, nullptr, nullptr, inst,
                                nullptr);
    // Icon dari resource yang sama dipakai judul jendela + taskbar, supaya
    // yang dilihat user di layar identik dengan icon file exe.
    g_iconBig = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(1), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
    g_iconSmall =
        (HICON)LoadImageW(inst, MAKEINTRESOURCEW(1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    if (g_iconBig) SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(g_iconBig));
    if (g_iconSmall)
        SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(g_iconSmall));

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
