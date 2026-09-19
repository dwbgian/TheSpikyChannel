#!/usr/bin/env python3
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets" / "generated"
MASCOT = Path("/Users/gianschreiner/Downloads/logoinverted.png")
WORDMARK = Path("/Users/gianschreiner/Downloads/a07965de-245e-435a-95c4-22677056d1c3.png")
W, H = 640, 480
FONT_FIRST = 32
FONT_LAST = 126


def font(size, bold=False):
    names = [
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf" if bold else "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Helvetica Bold.ttf" if bold else "/System/Library/Fonts/Supplemental/Helvetica.ttf",
        "/Library/Fonts/Arial.ttf",
    ]
    for name in names:
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return ImageFont.load_default()


def dark_mask(img):
    gray = img.convert("L")
    return gray.point(lambda p: max(0, min(255, 255 - p)) if p < 245 else 0)


def paste_logo(base, path, box, color, alpha=255):
    if not path.exists():
        return
    source = Image.open(path).convert("RGB")
    mask = dark_mask(source).filter(ImageFilter.GaussianBlur(0.2))
    logo = Image.new("RGBA", source.size, color + (alpha,))
    logo.putalpha(mask.point(lambda p: int(p * alpha / 255)))
    logo.thumbnail((box[2], box[3]), Image.Resampling.LANCZOS)
    base.alpha_composite(logo, (box[0], box[1]))


def build_background(connected):
    img = Image.new("RGBA", (W, H), (247, 249, 250, 255))
    d = ImageDraw.Draw(img, "RGBA")

    for y in range(H):
        shade = 255 - int(y * 0.028)
        d.line((0, y, W, y), fill=(shade, shade, shade, 255))

    # Clean Wii-menu-like wave background. No baked buttons or text panels.
    d.polygon([(0, 122), (165, 148), (370, 142), (640, 92),
               (640, 151), (380, 188), (170, 190), (0, 165)],
              fill=(239, 0, 18, 205))
    d.polygon([(0, 375), (170, 340), (370, 365), (640, 330),
               (640, 480), (0, 480)], fill=(224, 0, 15, 230))
    d.polygon([(0, 402), (200, 388), (414, 420), (640, 386),
               (640, 430), (392, 454), (160, 425), (0, 442)],
              fill=(255, 85, 96, 105))

    if MASCOT.exists():
        mark = Image.open(MASCOT).convert("RGB")
        mask = dark_mask(mark).filter(ImageFilter.GaussianBlur(1.0))
        opacity = 58 if connected else 34
        mark_rgba = Image.new("RGBA", mark.size, (220, 0, 15, opacity))
        mark_rgba.putalpha(mask.point(lambda p: int(p * opacity / 255)))
        mark_rgba.thumbnail((318, 318), Image.Resampling.LANCZOS)
        img.alpha_composite(mark_rgba, (344, 18))

    paste_logo(img, MASCOT, (30, 28, 76, 76), (12, 12, 14), 255)
    paste_logo(img, WORDMARK, (118, 30, 172, 62), (12, 12, 14), 255)

    light = (44, 200, 62, 255) if connected else (238, 0, 18, 255)
    d.ellipse((594, 34, 608, 48), fill=light)

    return img.convert("RGB")


def rgb_to_ycbcr(r, g, b):
    y = int(0.299 * r + 0.587 * g + 0.114 * b)
    cb = int(128 - 0.168736 * r - 0.331264 * g + 0.5 * b)
    cr = int(128 + 0.5 * r - 0.418688 * g - 0.081312 * b)
    return max(0, min(255, y)), max(0, min(255, cb)), max(0, min(255, cr))


def image_to_c_array(img, symbol):
    pixels = img.convert("RGB").load()
    words = []
    for y in range(H):
        for x in range(0, W, 2):
            r1, g1, b1 = pixels[x, y]
            r2, g2, b2 = pixels[x + 1, y]
            y1, cb1, cr1 = rgb_to_ycbcr(r1, g1, b1)
            y2, cb2, cr2 = rgb_to_ycbcr(r2, g2, b2)
            cb = (cb1 + cb2) // 2
            cr = (cr1 + cr2) // 2
            words.append((y1 << 24) | (cb << 16) | (y2 << 8) | cr)
    lines = [f"static const u32 {symbol}[{len(words)}] ATTRIBUTE_ALIGN(32) = {{"]
    for i in range(0, len(words), 8):
        chunk = ", ".join(f"0x{w:08x}" for w in words[i:i + 8])
        lines.append(f"    {chunk},")
    lines.append("};")
    return "\n".join(lines)


def build_font_arrays(symbol, pil_font, height):
    widths = []
    offsets = []
    bits = bytearray()

    for code in range(FONT_FIRST, FONT_LAST + 1):
        ch = chr(code)
        bbox = pil_font.getbbox(ch)
        width = max(3, min(32, bbox[2] - bbox[0] + 2))
        row_bytes = (width + 7) // 8
        offsets.append(len(bits))
        widths.append(width)

        glyph = Image.new("L", (width, height), 0)
        gd = ImageDraw.Draw(glyph)
        gd.text((1 - bbox[0], -bbox[1]), ch, font=pil_font, fill=255)

        for y in range(height):
            for byte_index in range(row_bytes):
                value = 0
                for bit in range(8):
                    x = byte_index * 8 + bit
                    if x < width and glyph.getpixel((x, y)) > 96:
                        value |= 0x80 >> bit
                bits.append(value)

    def c_list(values, per_line=16):
        lines = []
        for i in range(0, len(values), per_line):
            lines.append("    " + ", ".join(str(v) for v in values[i:i + per_line]) + ",")
        return "\n".join(lines)

    def c_hex(values, per_line=16):
        lines = []
        for i in range(0, len(values), per_line):
            lines.append("    " + ", ".join(f"0x{v:02x}" for v in values[i:i + per_line]) + ",")
        return "\n".join(lines)

    return f"""
static const u8 {symbol}_widths[{len(widths)}] = {{
{c_list(widths)}
}};

static const u32 {symbol}_offsets[{len(offsets)}] = {{
{c_list(offsets, 12)}
}};

static const u8 {symbol}_bits[{len(bits)}] ATTRIBUTE_ALIGN(32) = {{
{c_hex(bits)}
}};

static const SpikyFont {symbol} = {{
    {height},
    {symbol}_widths,
    {symbol}_offsets,
    {symbol}_bits
}};
"""


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    connected = build_background(True)
    disconnected = build_background(False)
    connected.save(OUT / "ui_connected_preview.png")
    disconnected.save(OUT / "ui_disconnected_preview.png")

    background_header = [
        "/* Generated by scripts/generate_red_ui.py. */",
        "#pragma once",
        "#include <gccore.h>",
        "#define SPIKY_UI_WIDTH 640",
        "#define SPIKY_UI_HEIGHT 480",
        image_to_c_array(connected, "spiky_ui_connected"),
        image_to_c_array(disconnected, "spiky_ui_disconnected"),
        "",
    ]
    (ROOT / "source" / "ui_background.h").write_text("\n\n".join(background_header))

    font_header = [
        "/* Generated by scripts/generate_red_ui.py. */",
        "#pragma once",
        "#include <gccore.h>",
        "#define SPIKY_FONT_FIRST 32",
        "#define SPIKY_FONT_LAST 126",
        "#define SPIKY_FONT_COUNT 95",
        "typedef struct SpikyFont {",
        "    u8 height;",
        "    const u8 *widths;",
        "    const u32 *offsets;",
        "    const u8 *bits;",
        "} SpikyFont;",
        build_font_arrays("spiky_font_body", font(15, True), 21),
        build_font_arrays("spiky_font_title", font(23, True), 31),
        "",
    ]
    (ROOT / "source" / "ui_font.h").write_text("\n".join(font_header))


if __name__ == "__main__":
    main()
