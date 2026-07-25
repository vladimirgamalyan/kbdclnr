<div align="center">

# 🧹 kbdclnr

**Keyboard lock for cleaning. One function — nothing else.**

![Windows](https://img.shields.io/badge/Windows-10%2F11-0078D6?logo=windows&logoColor=white)
![Language](https://img.shields.io/badge/C-WinAPI-A8B9CC?logo=c&logoColor=white)
![Size](https://img.shields.io/badge/exe-%E2%89%88110_KB-success)
![License](https://img.shields.io/badge/license-MIT-blue)

<img src="assets/screenshot.png" alt="kbdclnr window" width="584">

</div>

## Why

You want to wipe your keyboard without turning the computer off — but every
swipe over the keys types garbage into the active window. **kbdclnr** solves
exactly that:

1. **Run `kbdclnr.exe`** → all keys are disabled, the mouse keeps working.
2. Clean the keyboard as long as you like — the window stays on top
   as a reminder that the lock is active.
3. **Close the window** (the “Unlock and exit” button or the close button,
   with the mouse) → the keyboard works again.

## Download

A ready-to-run exe is on the [Releases](../../releases/latest) page.
No dependencies, no installation.

## How it works

The app installs a low-level [`WH_KEYBOARD_LL`](https://learn.microsoft.com/windows/win32/winmsg/lowlevelkeyboardproc)
hook and swallows every keyboard event (returns `1` from the hook). On exit
the hook is removed. If the process dies unexpectedly, Windows removes the
hook automatically — the keyboard can never stay locked.

## Limitations

- **`Ctrl+Alt+Del` is not blocked** — Windows handles the Secure Attention
  Sequence before any hooks. It doesn't get in the way of cleaning.
- The on-screen keyboard and other software input sources are blocked too
  (the hook swallows injected events as well).

## Building

All you need is clang (or any C compiler with the Windows SDK):

```sh
clang -O2 kbdclnr.c -o kbdclnr.exe -luser32 -lgdi32 "-Wl,/SUBSYSTEM:WINDOWS"
```

## License

MIT
