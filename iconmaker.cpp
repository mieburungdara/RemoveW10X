// iconmaker.cpp - pembuat app.ico (dev tool).
// iconmaker build <out.ico>          : gambar semua ukuran + tulis ICO
// iconmaker preview <out.png>        : lembar pratinjau semua ukuran
// iconmaker check <exe> <out.png>    : ambil icon dari dalam exe (verifikasi)
//
// Bendera/icon digambar vektor-ish dengan GDI+, bukan dari emoji atau file
// eksternal, supaya tajam di 16px dan tetap mulus di 256px.
#include <windows.h>

#include <cstdio>
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include <gdiplus.h>

using namespace Gdiplus;

#pragma pack(push, 2)
struct IcoDir { WORD reserved, type, count; };
struct IcoEntry {
    BYTE w, h, color, reserved;
    WORD planes, bitCount;
    DWORD size, offset;
};
#pragma pack(pop)

static const int kSizes[] = {16, 24, 32, 48, 64, 128, 256};
static const int kCount = sizeof(kSizes) / sizeof(kSizes[0]);

static Status SavePngTo(Bitmap* bmp, const wchar_t* path) {
    CLSID clsid{};
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    std::string buf(size, '\0');
    ImageCodecInfo* info = reinterpret_cast<ImageCodecInfo*>(buf.data());
    GetImageEncoders(num, size, info);
    bool found = false;
    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(info[i].MimeType, L"image/png") == 0) {
            clsid = info[i].Clsid;
            found = true;
            break;
        }
    }
    if (!found) return GenericError;
    return bmp->Save(path, &clsid, nullptr);
}

// Jendela dengan tombol close yang dicoret: inti dari kegunaan aplikasi ini.
static void DrawIcon(HDC dc, int S) {
    Graphics g(dc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.Clear(Color(0, 0, 0, 0));

    const float m = S * 0.07f;
    const float x0 = m, y0 = S * 0.14f, x1 = S - m, y1 = S - m * 0.8f;
    const float w = x1 - x0, h = y1 - y0;
    const float rad = S * 0.10f;
    const float bar = h * 0.26f;

    auto roundRect = [&](GraphicsPath& p, float x, float y, float ww, float hh, float r) {
        p.AddArc(x, y, r * 2, r * 2, 180, 90);
        p.AddArc(x + ww - r * 2, y, r * 2, r * 2, 270, 90);
        p.AddArc(x + ww - r * 2, y + hh - r * 2, r * 2, r * 2, 0, 90);
        p.AddArc(x, y + hh - r * 2, r * 2, r * 2, 90, 90);
        p.CloseFigure();
    };

    // Badan jendela.
    GraphicsPath body;
    roundRect(body, x0, y0, w, h, rad);
    g.SetClip(&body);
    SolidBrush bodyFill(Color(255, 243, 246, 250));
    g.FillRectangle(&bodyFill, RectF(x0, y0, w, h));
    // Title bar.
    SolidBrush barFill(Color(255, 40, 56, 82));
    g.FillRectangle(&barFill, RectF(x0, y0, w, bar));
    // Isi: dua garis teks tipis biar terbaca sebagai jendela.
    if (S >= 32) {
        SolidBrush line(Color(255, 203, 213, 225));
        float ly = y0 + bar + h * 0.14f;
        for (int i = 0; i < 3; ++i) {
            float lw = (i == 2) ? w * 0.45f : w * 0.72f;
            g.FillRectangle(&line, RectF(x0 + w * 0.10f, ly, lw, std::max(1.0f, S * 0.025f)));
            ly += h * 0.16f;
        }
    }
    g.ResetClip();

    // Tombol-tombol caption di kanan title bar.
    const float slot = S * (S >= 48 ? 0.13f : 0.15f);
    const float inset = S * 0.17f;
    const float glyph = slot * 0.52f;
    float gx = x1 - inset;
    float gy = y0 + bar / 2.0f;
    Pen ink(Color(255, 198, 210, 226)), thick(Color(255, 198, 210, 226), std::max(1.0f, S / 24.0f));

    if (S >= 32) {
        // minimize
        g.DrawLine(&thick, gx - slot * 1.9f, gy + glyph * 0.4f, gx - slot * 1.9f + glyph,
                   gy + glyph * 0.4f);
        // maximize
        g.DrawRectangle(&ink, RectF(gx - slot * 0.95f - glyph / 2, gy - glyph / 2, glyph, glyph));
    }
    // close (X) - digambar pucat karena inilah yang kita hilangkan
    Pen ghost(Color(150, 255, 255, 255), std::max(1.0f, S / 20.0f));
    float cx = gx, cy = gy;
    g.DrawLine(&ghost, cx - glyph / 2, cy - glyph / 2, cx + glyph / 2, cy + glyph / 2);
    g.DrawLine(&ghost, cx + glyph / 2, cy - glyph / 2, cx - glyph / 2, cy + glyph / 2);

    // Lingkaran coret (larangan) di atas X.
    float rr = S * 0.165f;
    Pen halo(Color(255, 246, 249, 252), std::max(2.2f, S / 8.0f));
    Pen ban(Color(255, 214, 42, 34), std::max(1.6f, S / 13.0f));
    g.DrawEllipse(&halo, RectF(cx - rr, cy - rr, rr * 2, rr * 2));
    g.DrawEllipse(&ban, RectF(cx - rr, cy - rr, rr * 2, rr * 2));
    const double a = 0.7853981634;  // 45 derajat
    REAL dx = rr * (REAL)cos(a), dy = rr * (REAL)sin(a);
    g.DrawLine(&halo, cx - dx, cy - dy, cx + dx, cy + dy);
    g.DrawLine(&ban, cx - dx, cy - dy, cx + dx, cy + dy);

    // Bingkai jendela.
    Pen frame(Color(255, 58, 69, 84), std::max(1.0f, S / 40.0f));
    g.DrawPath(&frame, &body);
}

static Bitmap* MakeBitmap(int S, HDC* outDc) {
    HDC screen = GetDC(nullptr);
    HDC dc = CreateCompatibleDC(screen);
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = S;
    bmi.bmiHeader.biHeight = -S;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HBITMAP old = (HBITMAP)SelectObject(dc, dib);
    DrawIcon(dc, S);
    SelectObject(dc, old);
    DeleteDC(dc);
    ReleaseDC(nullptr, screen);
    *outDc = nullptr;
    Bitmap* bmp = Bitmap::FromHBITMAP(dib, nullptr);
    DeleteObject(dib);
    return bmp;
}

static std::vector<char> ReadBytes(const wchar_t* path) {
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return {};
    DWORD n = GetFileSize(h, nullptr);
    std::vector<char> out(n);
    DWORD got = 0;
    ::ReadFile(h, out.data(), n, &got, nullptr);
    out.resize(got);
    CloseHandle(h);
    return out;
}

static int BuildIco(const wchar_t* outPath) {
    std::wstring tmp = outPath;
    tmp += L".tmp";
    std::vector<std::vector<char>> blobs;
    for (int S : kSizes) {
        HDC ignored = nullptr;
        Bitmap* bmp = MakeBitmap(S, &ignored);
        std::wstring p = tmp + L"-" + std::to_wstring(S) + L".png";
        Status st = SavePngTo(bmp, p.c_str());
        delete bmp;
        if (st != Ok) {
            wprintf(L"GAGAL simpan png ukuran %d (status %d)\n", S, (int)st);
            return 1;
        }
        blobs.push_back(ReadBytes(p.c_str()));
        DeleteFileW(p.c_str());
    }

    IcoDir dir{};
    dir.reserved = 0;
    dir.type = 1;
    dir.count = (WORD)blobs.size();
    std::vector<IcoEntry> ents(blobs.size());
    DWORD off = sizeof(dir) + sizeof(IcoEntry) * blobs.size();
    for (size_t i = 0; i < blobs.size(); ++i) {
        int S = kSizes[i];
        ents[i].w = (S >= 256) ? 0 : (BYTE)S;
        ents[i].h = (S >= 256) ? 0 : (BYTE)S;
        ents[i].color = 0;
        ents[i].reserved = 0;
        ents[i].planes = 1;
        ents[i].bitCount = 32;
        ents[i].size = (DWORD)blobs[i].size();
        ents[i].offset = off;
        off += (DWORD)blobs[i].size();
    }

    HANDLE h = CreateFileW(outPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                           nullptr);
    if (h == INVALID_HANDLE_VALUE) return 1;
    DWORD wrote = 0;
    WriteFile(h, &dir, sizeof(dir), &wrote, nullptr);
    WriteFile(h, ents.data(), (DWORD)(sizeof(IcoEntry) * ents.size()), &wrote, nullptr);
    for (auto& b : blobs) WriteFile(h, b.data(), (DWORD)b.size(), &wrote, nullptr);
    CloseHandle(h);

    for (size_t i = 0; i < blobs.size(); ++i) {
        wprintf(L"  %3dx%-3d %8zu byte\n", kSizes[i], kSizes[i], blobs[i].size());
    }
    return 0;
}

static int Preview(const wchar_t* outPath) {
    const int cell = 64, pad = 6;
    const int W = kCount * (cell + pad) + pad, H = cell + pad * 2;
    HDC screen = GetDC(nullptr);
    HDC dc = CreateCompatibleDC(screen);
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = W;
    bmi.bmiHeader.biHeight = -H;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    SelectObject(dc, dib);
    Graphics g(dc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.Clear(Color(255, 226, 229, 236));
    for (int i = 0; i < kCount; ++i) {
        HDC ignored = nullptr;
        Bitmap* bmp = MakeBitmap(kSizes[i], &ignored);
        int draw = std::min(kSizes[i], cell);
        g.DrawImage(bmp, pad + i * (cell + pad) + (cell - draw) / 2, pad, draw, draw);
        delete bmp;
    }
    Bitmap* out = Bitmap::FromHBITMAP(dib, nullptr);
    Status st = SavePngTo(out, outPath);
    delete out;
    DeleteObject(dib);
    DeleteDC(dc);
    ReleaseDC(nullptr, screen);
    wprintf(L"preview status=%d\n", (int)st);
    return st == Ok ? 0 : 1;
}

static int CheckExe(const wchar_t* exe, const wchar_t* outPath) {
    HICON big = nullptr, small = nullptr;
    UINT n = ExtractIconExW(exe, 0, &big, &small, 1);
    if (n == 0 || !big) {
        wprintf(L"EXE tidak punya icon (n=%u)\n", n);
        return 1;
    }
    const int S = 128;
    HDC screen = GetDC(nullptr);
    HDC dc = CreateCompatibleDC(screen);
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = S;
    bmi.bmiHeader.biHeight = -S;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    SelectObject(dc, dib);
    Graphics g(dc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.Clear(Color(255, 226, 229, 236));
    // Header GDI+ MinGW tidak mengekspos Graphics::DrawIcon, jadi pakai Win32.
    DrawIconEx(dc, 0, 0, big, S, S, 0, nullptr, DI_NORMAL);
    Bitmap* out = Bitmap::FromHBITMAP(dib, nullptr);
    Status st = SavePngTo(out, outPath);
    delete out;
    DeleteObject(dib);
    DeleteDC(dc);
    ReleaseDC(nullptr, screen);
    DestroyIcon(big);
    if (small) DestroyIcon(small);
    wprintf(L"icon dari exe: status=%d\n", (int)st);
    return st == Ok ? 0 : 1;
}

int wmain(int argc, wchar_t** argv) {
    if (argc < 3) {
        wprintf(L"pakai: iconmaker build <out.ico> | preview <out.png> | check <exe> <out.png>\n");
        return 2;
    }
    std::wstring mode = argv[1];
    GdiplusStartupInput gsi;
    ULONG_PTR token = 0;
    if (GdiplusStartup(&token, &gsi, nullptr) != Ok) return 1;
    int rc = 1;
    if (mode == L"build") rc = BuildIco(argv[2]);
    else if (mode == L"preview") rc = Preview(argv[2]);
    else if (mode == L"check" && argc >= 4) rc = CheckExe(argv[2], argv[3]);
    GdiplusShutdown(token);
    return rc;
}
