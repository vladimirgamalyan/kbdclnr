# 0001. Embed the window image as a PNG resource decoded with WIC

- Status: Accepted
- Date: 2026-07-25

## Context

The window used to contain a title, a description and an "Unlock and exit"
button. It now shows a single image and nothing else; the window is closed with
the title bar close button.

The image has to ship inside the exe — the project promises a single file with
no dependencies and no installation. Two encodings were considered:

- **BMP as a `BITMAP` resource.** Loads with a single `LoadImage` call, but the
  data is uncompressed: a 480×240 24-bit image costs ~338 KB, tripling the exe.
- **PNG as an `RCDATA` resource.** Roughly 8 KB for the same image, at the cost
  of decoding it at runtime.

For decoding, GDI+ is the usual answer, but `gdiplus.h` is a C++ header and
this project is plain C. Using its flat API would mean hand-declaring system
prototypes and structs. WIC is COM and exposes C interfaces via `COBJMACROS`,
so it needs no such workaround.

## Decision

Store the image as `assets/window.png`, compile it into the exe as an `RCDATA`
resource, and decode it at startup with WIC into a top-down 32-bit DIB. Paint
it with `StretchBlt` over the whole client area.

The client area stays at 480×240 logical pixels (DPI-scaled) regardless of the
image's pixel size, so replacing the image never changes the window geometry.

## Consequences

- The exe grows by the compressed size of the image (~21 KB for the current
  one) instead of the ~340 KB an uncompressed 480×240 BMP would cost, and the
  image is replaced by swapping one PNG file and rebuilding.
- The build gained a resource-compilation step (`llvm-rc kbdclnr.rc`) and three
  link-time libraries: `ole32`, `shlwapi`, `windowscodecs`.
- ~50 lines of COM boilerplate now sit in a program that is otherwise
  straightforward WinAPI.
- Because the image is stretched to a fixed logical size, an image whose aspect
  ratio is not 2:1 is distorted, and a low-resolution image looks soft on a
  high-DPI display.
