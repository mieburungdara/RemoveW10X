#pragma once
#include <windows.h>

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

struct Win {
    HWND hwnd;
    std::wstring title;
    std::wstring proc;
};

inline std::wstring ToLower(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](wchar_t c) { return static_cast<wchar_t>(towlower(c)); });
    return s;
}

inline std::wstring WindowTitle(HWND hwnd) {
    int len = GetWindowTextLengthW(hwnd);
    std::wstring t(len + 1, L'\0');
    int n = GetWindowTextW(hwnd, t.data(), len + 1);
    t.resize(n > 0 ? n : 0);
    return t;
}

inline std::wstring ProcName(HWND hwnd) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return L"?";
    wchar_t buf[MAX_PATH];
    DWORD n = MAX_PATH;
    std::wstring out = L"?";
    if (QueryFullProcessImageNameW(h, 0, buf, &n)) {
        std::wstring p(buf, n);
        auto pos = p.find_last_of(L"\\/");
        out = (pos == std::wstring::npos) ? p : p.substr(pos + 1);
    }
    CloseHandle(h);
    return out;
}

inline BOOL CALLBACK TopWindowProc(HWND hwnd, LPARAM lp) {
    if (!IsWindowVisible(hwnd)) return TRUE;
    if (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) return TRUE;
    std::wstring title = WindowTitle(hwnd);
    if (title.empty()) return TRUE;
    reinterpret_cast<std::vector<Win>*>(lp)->push_back({hwnd, title, ProcName(hwnd)});
    return TRUE;
}

inline std::vector<Win> AllTopWindows() {
    std::vector<Win> v;
    EnumWindows(TopWindowProc, reinterpret_cast<LPARAM>(&v));
    return v;
}

// Kecocokan persis lebih diutamakan daripada substring.
inline HWND FindByTitle(const wchar_t* sub) {
    std::wstring needle = ToLower(sub);
    HWND exact = nullptr, partial = nullptr;
    for (const Win& w : AllTopWindows()) {
        std::wstring t = ToLower(w.title);
        if (t == needle) exact = w.hwnd;
        else if (!partial && t.find(needle) != std::wstring::npos) partial = w.hwnd;
    }
    return exact ? exact : partial;
}

// "#1a2b3c" = hwnd heksadesimal, supaya tidak ada ambiguitas pencocokan judul.
inline HWND Resolve(const wchar_t* arg) {
    if (arg[0] == L'#') {
        HWND hwnd =
            reinterpret_cast<HWND>(static_cast<ULONG_PTR>(wcstoull(arg + 1, nullptr, 16)));
        return IsWindow(hwnd) ? hwnd : nullptr;
    }
    return FindByTitle(arg);
}
