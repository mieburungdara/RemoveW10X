#include "winfind.h"
#include <cstdio>

static BOOL CALLBACK Proc(HWND hwnd, LPARAM lp) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    auto* want = reinterpret_cast<DWORD*>(lp);
    if (pid != *want) return TRUE;
    LONG_PTR st = GetWindowLongPtrW(hwnd, GWL_STYLE);
    LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    RECT rc{};
    GetWindowRect(hwnd, &rc);
    wchar_t cn[128]{};
    GetClassNameW(hwnd, cn, 128);
    std::wstring t = WindowTitle(hwnd);
    wprintf(L"hwnd=%p vis=%d class=%-20ls cap=%d sysmenu=%d min=%d max=%d thick=%d "
            L"ex=%08lx rect=%ld,%ld %ldx%ld title=\"%ls\"\n",
            (void*)hwnd, IsWindowVisible(hwnd) ? 1 : 0, cn, ((st & WS_CAPTION) == WS_CAPTION) ? 1 : 0,
            (st & WS_SYSMENU) ? 1 : 0, (st & WS_MINIMIZEBOX) ? 1 : 0,
            (st & WS_MAXIMIZEBOX) ? 1 : 0, (st & WS_THICKFRAME) ? 1 : 0, (unsigned long)ex, rc.left,
            rc.top, rc.right - rc.left, rc.bottom - rc.top, t.c_str());
    return TRUE;
}

int wmain(int argc, wchar_t** argv) {
    if (argc < 2) {
        wprintf(L"pakai: allwins <nama-proses.exe>\n");
        return 2;
    }
    for (const Win& w : AllTopWindows()) {
        if (_wcsicmp(w.proc.c_str(), argv[1]) != 0) continue;
        DWORD pid = 0;
        GetWindowThreadProcessId(w.hwnd, &pid);
        wprintf(L"=== %ls (pid %lu) ===\n", w.title.c_str(), pid);
        EnumWindows(Proc, reinterpret_cast<LPARAM>(&pid));
        return 0;
    }
    wprintf(L"proses \"%ls\" tidak punya jendela terlihat\n", argv[1]);
    return 1;
}
