@echo off
rem Build "Hilangkan tombol close". Butuh MinGW-w64 g++ di PATH.
rem Mesin ini: C:\Users\Administrator\toolchains\mingw64\bin
rem Theming widget datang dari manifest default MinGW (comctl32 v6 + asInvoker),
rem jadi tidak perlu file .manifest/.rc sendiri.
cd /d %~dp0

set FLAGS=-std=c++20 -O2 -municode -static

rem Aplikasi utama (statis: tidak butuh DLL MinGW saat dijalankan)
g++ %FLAGS% -mwindows main.cpp -o CloseButtonTool.exe -luser32 -lgdi32 -lcomctl32 || exit /b 1

rem Harness verifikasi (opsional)
g++ %FLAGS% probe.cpp -o probe.exe -luser32 -lgdi32 || exit /b 1
g++ %FLAGS% snap.cpp -o snap.exe -luser32 -lgdi32 -lgdiplus || exit /b 1
g++ %FLAGS% drive.cpp -o drive.exe -luser32 || exit /b 1

echo Build selesai: CloseButtonTool.exe
