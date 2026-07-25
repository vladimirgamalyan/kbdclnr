# /// script
# requires-python = ">=3.11"
# dependencies = ["pillow"]
# ///
"""Build assets/icon.ico from assets/window.png.

Run after replacing the window image:

    uv run --script tools/make_icon.py
"""

import io
import struct
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "assets" / "window.png"
DST = ROOT / "assets" / "icon.ico"

SIZES = [16, 24, 32, 48, 64, 128, 256]
CORNER_RADIUS = 0.22  # fraction of the tile side


def ink_crop(img):
    """Crop to the bounding box of the drawing, dropping the white border."""
    gray = img.convert("L")
    return img.crop(gray.point(lambda v: 255 if v < 240 else 0).getbbox())


def render(art, size):
    """Draw the artwork centred on a white rounded tile of the given size."""
    margin = 0.06 if size <= 32 else 0.10
    box = round(size * (1 - 2 * margin))
    w, h = art.size
    scale = min(box / w, box / h)
    tw, th = round(w * scale), round(h * scale)

    # Thin strokes wash out when downscaled this far, so fatten them first.
    src = art.filter(ImageFilter.MinFilter(3)) if size <= 48 else art
    small = src.resize((tw, th), Image.LANCZOS)

    tile = Image.new("RGBA", (size, size), (255, 255, 255, 255))
    ss = 8  # supersample the corner mask for smooth edges
    mask = Image.new("L", (size * ss, size * ss), 0)
    ImageDraw.Draw(mask).rounded_rectangle(
        (0, 0, size * ss - 1, size * ss - 1), radius=size * ss * CORNER_RADIUS, fill=255
    )
    tile.putalpha(mask.resize((size, size), Image.LANCZOS))
    tile.alpha_composite(small, ((size - tw) // 2, (size - th) // 2))

    # The art is black on white: 16 grey levels are visually identical here and
    # nearly halve the PNG. The frame stays 32bpp RGBA, as Windows expects.
    flat = tile.convert("RGB").quantize(colors=16).convert("RGB")
    flat.putalpha(tile.getchannel("A"))
    return flat


def write_ico(path, frames):
    """Write an ICO whose frames are PNG-compressed (supported since Vista)."""
    blobs = []
    for frame in frames:
        buf = io.BytesIO()
        frame.save(buf, "PNG", optimize=True)
        blobs.append(buf.getvalue())

    out = bytearray(struct.pack("<HHH", 0, 1, len(frames)))  # reserved, type, count
    offset = 6 + 16 * len(frames)
    for frame, blob in zip(frames, blobs):
        side = 0 if frame.width >= 256 else frame.width  # 0 means 256
        out += struct.pack("<BBBBHHII", side, side, 0, 0, 1, 32, len(blob), offset)
        offset += len(blob)
    for blob in blobs:
        out += blob
    path.write_bytes(bytes(out))


def main():
    art = ink_crop(Image.open(SRC).convert("RGBA"))
    write_ico(DST, [render(art, size) for size in SIZES])
    print(f"{DST.relative_to(ROOT)}: {len(SIZES)} sizes, {DST.stat().st_size} bytes")


if __name__ == "__main__":
    main()
