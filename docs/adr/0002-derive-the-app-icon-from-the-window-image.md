# 0002. Derive the app icon from the window image

- Status: Accepted
- Date: 2026-07-25

## Context

The app shipped with the stock `IDI_APPLICATION` icon, so the exe, the taskbar
and the title bar showed a generic Windows icon. The window already carries the
project's only piece of artwork — the drawing in `assets/window.png` — and
there is no reason for the icon to show anything else.

The drawing is a wide (about 3:2 after cropping the white border) black line
sketch, which raises two problems for an icon: it has to fit a square, and thin
strokes turn into pale mush when downscaled to 16 px.

An icon is also pure data, and this project watches the exe size closely (see
ADR-0001). A naive 7-size 32-bit ICO with BMP frames costs over 100 KB.

## Decision

Generate `assets/icon.ico` from `assets/window.png` with
`tools/make_icon.py` and compile it into the exe as an `ICON` resource. The
window class sets both `hIcon` and `hIconSm`, so the title bar gets the 16 px
frame instead of a downscaled 32 px one — which is why `WNDCLASSW` became
`WNDCLASSEXW`.

The generator crops the drawing to its ink bounding box, centres it on a white
rounded tile (transparent outside the corners), thickens the strokes with a
`MinFilter` for frames of 48 px and below, and quantises each frame to 16 grey
levels. Frames are PNG-compressed inside the ICO, which Windows has supported
since Vista.

The white tile is deliberate: the artwork is black lines with no fill, so a
transparent background would make the icon invisible on a dark taskbar.

## Consequences

- The icon is a derived asset. Replacing `assets/window.png` means re-running
  `uv run --script tools/make_icon.py`, otherwise the icon keeps showing the
  old drawing.
- The project gained a Python/Pillow build-time dependency, but only for
  regenerating the icon — building the exe still needs nothing but clang.
- The ICO holds 7 sizes (16–256) for ~29 KB; the exe grows from ~132 KB to
  ~162 KB.
- PNG-compressed ICO frames are not readable by pre-Vista Windows. The project
  targets Windows 10/11, so this costs nothing.
- At 16 px the drawing is a silhouette — the figure and the keyboard are
  recognisable, the individual keys are not. That is the ceiling for artwork
  this detailed.
