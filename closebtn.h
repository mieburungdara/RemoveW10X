#pragma once
#include <windows.h>

#include <string>

// Keadaan item SC_CLOSE di system menu sebuah jendela.
enum class CloseState {
    NoSystemMenu,
    Removed,
    Disabled,
    Enabled,
};

inline const wchar_t* StateName(CloseState s) {
    switch (s) {
        case CloseState::NoSystemMenu: return L"tidak punya system menu";
        case CloseState::Removed:      return L"HILANG (item dihapus)";
        case CloseState::Disabled:     return L"ABU-ABU (disabled)";
        case CloseState::Enabled:      return L"AKTIF";
    }
    return L"?";
}

inline std::wstring LastErrorText() {
    DWORD err = GetLastError();
    if (!err) return L"OK";
    wchar_t* buf = nullptr;
    DWORD n = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                             nullptr, err, 0, reinterpret_cast<wchar_t*>(&buf), 0, nullptr);
    std::wstring out = std::to_wstring(err);
    if (n && buf) {
        out += L" ";
        out += buf;
    }
    if (buf) LocalFree(buf);
    while (!out.empty() && (out.back() == L'\n' || out.back() == L'\r')) out.pop_back();
    return out;
}

// Hasil pembacaan menu. PENTING (terukur di mesin ini): GetSystemMenu untuk
// jendela proses lain kadang mengembalikan handle yang tidak bisa dipakai dari
// proses ini (GetMenuState gagal, err 1401). Kalau begitu, status SC_CLOSE tidak
// diketahui dan upaya mengubahnya tidak akan berpengaruh ke target.
struct MenuProbe {
    bool hasStyle = false;
    bool readable = false;
    CloseState state = CloseState::NoSystemMenu;
    DWORD err = 0;
    int items = -1;
};

inline MenuProbe ProbeClose(HWND hwnd) {
    MenuProbe p;
    p.hasStyle = (GetWindowLongPtrW(hwnd, GWL_STYLE) & WS_SYSMENU) != 0;
    if (!p.hasStyle) {
        p.state = CloseState::NoSystemMenu;
        p.err = GetLastError();
        return p;
    }
    SetLastError(0);
    HMENU menu = GetSystemMenu(hwnd, FALSE);
    if (!menu) {
        p.err = GetLastError();
        p.state = CloseState::NoSystemMenu;
        return p;
    }
    p.items = GetMenuItemCount(menu);
    SetLastError(0);
    UINT st = GetMenuState(menu, SC_CLOSE, MF_BYCOMMAND);
    p.err = GetLastError();
    if (st == static_cast<UINT>(-1)) {
        p.readable = (p.err == 0);
        p.state = CloseState::Removed;
    } else {
        p.readable = true;
        p.state = (st & MF_GRAYED) ? CloseState::Disabled : CloseState::Enabled;
    }
    return p;
}

inline std::wstring DescribeMenu(const MenuProbe& p) {
    std::wstring s = StateName(p.state);
    if (!p.readable && p.hasStyle) {
        s += L" - TIDAK TERBACA dari proses ini (handle menu milik proses target)";
    }
    s += L", item=" + std::to_wstring(p.items) + L", err=" + std::to_wstring(p.err);
    return s;
}

// Memaksa caption di-render ulang oleh jendela target. DrawMenuBar adalah jalur
// resminya; SetWindowPos(FRAMECHANGED) sebagai cadangan kalau gagal.
inline bool RepaintCaption(HWND hwnd) {
    if (DrawMenuBar(hwnd)) return true;
    return SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                            SWP_FRAMECHANGED) != 0;
}

// Mode 1: hanya tombol X (lewat item SC_CLOSE di system menu).
inline bool RemoveCloseButton(HWND hwnd, bool deleteItem) {
    HMENU menu = GetSystemMenu(hwnd, FALSE);
    if (!menu) return false;
    SetLastError(0);
    BOOL ok;
    if (deleteItem) {
        ok = RemoveMenu(menu, SC_CLOSE, MF_BYCOMMAND);
    } else {
        ok = EnableMenuItem(menu, SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    }
    if (!ok) return false;
    RepaintCaption(hwnd);
    return true;
}

// GetSystemMenu(hwnd, TRUE) membuang salinan system menu dan mengembalikan
// versi default. Efek samping terukur: handle yang sudah dipegang proses lain
// jadi tidak valid, jadi daftar status perlu dibaca ulang setelah ini.
inline bool RestoreCloseButton(HWND hwnd) {
    SetLastError(0);
    HMENU menu = GetSystemMenu(hwnd, TRUE);
    if (!menu) return false;
    RepaintCaption(hwnd);
    return true;
}

// Mode 2: style jendela disimpan win32k di objek jendela sehingga bisa diubah
// lintas proses. Menghapus WS_SYSMENU menghilangkan SEMUA tombol caption
// (minimize, maximize, close) - terbukti secara visual pada jendela milik proses
// lain, dan bisa dibalik.
inline bool SetSysMenuStyle(HWND hwnd, bool enable) {
    LONG_PTR cur = GetWindowLongPtrW(hwnd, GWL_STYLE);
    LONG_PTR want = enable ? (cur | WS_SYSMENU) : (cur & ~static_cast<LONG_PTR>(WS_SYSMENU));
    if (cur == want) return true;
    SetLastError(0);
    if (SetWindowLongPtrW(hwnd, GWL_STYLE, want) == 0 && GetLastError() != 0) return false;
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    return true;
}
