// snap.cpp - harness verifikasi visual (dev tool).
// Menangkap jendela target (atau strip atasnya saja) ke PNG supaya tombol close
// bisa dilihat langsung, bukan disimpulkan dari return value API.
#include <windows.h>

#include <cstdio>
#include <string>

#include <gdiplus.h>

#include "winfind.h"

#ifndef PW_RENDERFULLCONTENT
#define PW_RENDERFULLCONTENT 0x00000002
#endif

static bool GetEncoderClsid(const wchar_t* format, CLSID* clsid) {
    using namespace Gdiplus;
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    if (!size) return false;
    std::string buf(size, '\0');
    ImageCodecInfo* info = reinterpret_cast<ImageCodecInfo*>(buf.data());
    GetImageEncoders(num, size, info);
    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(info[i].MimeType, format) == 0) {
            *clsid = info[i].Clsid;
            return true;
        }
    }
    return false;
}

int wmain(int argc, wchar_t** argv) {
    SetProcessDPIAware();
    if (argc < 3) {
        wprintf(L"pakai: snap <judul|#hwnd> <out.png> [tinggi_strip_px]\n");
        return 2;
    }
    HWND hwnd = Resolve(argv[1]);
    if (!hwnd) {
        wprintf(L"jendela \"%s\" tidak ditemukan\n", argv[1]);
        return 1;
    }
    RECT rc{};
    if (!GetWindowRect(hwnd, &rc)) return 1;
    if (argc > 3) {
        int strip = _wtoi(argv[3]);
        if (strip > 0) rc.bottom = std::min(rc.top + strip, rc.bottom);
    }
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) {
        wprintf(L"ukuran jendela tidak valid (%dx%d)\n", w, h);
        return 1;
    }

    HDC screen = GetDC(nullptr);
    HDC mem = CreateCompatibleDC(screen);
    HBITMAP bmp = CreateCompatibleBitmap(screen, w, h);
    HGDIOBJ old = SelectObject(mem, bmp);
    // PrintWindow mengambil isi jendela walau tertutup jendela lain.
    BOOL printed = PrintWindow(hwnd, mem, PW_RENDERFULLCONTENT);
    if (!printed) BitBlt(mem, 0, 0, w, h, screen, rc.left, rc.top, SRCCOPY);
    SelectObject(mem, old);
    DeleteDC(mem);
    ReleaseDC(nullptr, screen);

    using namespace Gdiplus;
    GdiplusStartupInput gsi;
    ULONG_PTR token = 0;
    if (GdiplusStartup(&token, &gsi, nullptr) != Ok) return 1;
    CLSID clsid{};
    bool encoded = GetEncoderClsid(L"image/png", &clsid);
    Bitmap* pb = Bitmap::FromHBITMAP(bmp, nullptr);
    Status st = pb ? pb->Save(argv[2], &clsid, nullptr) : Gdiplus::InvalidParameter;
    delete pb;
    DeleteObject(bmp);
    GdiplusShutdown(token);

    wprintf(L"capture %dx%d hwnd=%p cara=%s status=%d\n", w, h, (void*)hwnd,
            printed ? "PrintWindow" : "BitBlt-layar(RISIKO-OCCLUDED)", (int)st);
    return (st == Gdiplus::Ok && encoded && printed) ? 0 : 3;
}
