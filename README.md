# RemoveW10X

[**Bahasa Indonesia**](README.id.md) · English

Hide the close button of **another application's window** on Windows 10/11 x64.
A single small native exe — no installer, no runtime dependencies.

![Icon](bukti/icon.png)

## Run it

Download [`RemoveW10X.exe`](RemoveW10X.exe) and double-click it. Nothing to install.

Windows SmartScreen may warn on first run because the binary is unsigned — choose
**More info → Run anyway**.

## What it does

Pick a window from the dropdown, then click an action. Both actions are reversible
with **Restore**.

| Button | Mechanism | Effect |
|---|---|---|
| **Remove X button only** | deletes the `SC_CLOSE` item from the target's system menu | only the close button greys out; minimize and maximize stay alive |
| **Hide all caption buttons** | clears the `WS_SYSMENU` window style | minimize, maximize **and** close all disappear together |

Verified on live windows: the system menu item count goes `7 → 6` and the target's
caption repaints with a dead close button.

## Limits — read this before relying on it

- **This only stops the click on the button.** `Alt+F4`, "Close window" from the
  taskbar, and the target application's own shutdown path still close the window.
  Measured: sending `WM_SYSCOMMAND / SC_CLOSE` to a window whose X we had removed
  still killed it. A hard lock would require a message hook injected into the
  target process, which this tool deliberately does not do.
- **Elevated windows are refused.** A non-admin process cannot change a window
  owned by an admin process; the tool reports the failure instead of pretending.
  Run it as administrator if you need those.
- **Apps that draw their own title bar** (Chrome, Discord, Electron apps) have no
  Win32 caption to modify. The tool says so per window rather than silently doing
  nothing.
- Some windows hand back a system-menu handle that is only valid inside the target
  process (`err 1401`). The tool then resets the menu to get a usable copy and
  retries; if that still fails it tells you to use the second mode.

## Language

Two flag buttons at the top right switch the whole UI — labels, buttons and log —
between Bahasa Indonesia and English. The default follows your Windows UI language.
Flags are drawn with GDI because Win32 GDI does not render flag emoji as glyphs.

## Build from source

Requires MinGW-w64 `g++` on `PATH` (MSVC is not needed):

```bat
build.bat
```

The build is `-static` on purpose: this machine has two MinGW installs on `PATH`,
and a dynamically linked binary died at load with `0xC0000139` (entry point not
found). Static output runs anywhere.

## Files

| File | Role |
|---|---|
| `main.cpp` | UI, window list, both modes, i18n |
| `closebtn.h` | the actual Win32 calls + honest status probing |
| `winfind.h` | top-level window enumeration and lookup |
| `app.rc`, `app.ico` | exe icon (see `iconmaker.cpp`) |
| `iconmaker.cpp` | draws the icon at 7 sizes and packs a valid ICO |
| `probe.cpp` | CLI harness: state / hide / gray / restore / nostyle / tryclose |
| `snap.cpp` | captures a window with `PrintWindow` to PNG |
| `drive.cpp` | clicks the app's own buttons and dumps its log, for testing |
| `bukti/` | visual evidence of the two modes and the icon |

The harnesses are how the claims above were measured, e.g.:

```bat
probe.exe state "Untitled - Notepad"
probe.exe hide  "Untitled - Notepad"
probe.exe tryclose "Untitled - Notepad"
```

`RemoveW10X.exe` is committed so people can run it directly; rebuild it after
editing the sources and the binary will show up as a diff.
