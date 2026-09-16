#!/usr/bin/env python3
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import math

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets" / "generated"
MASCOT = Path("/Users/gianschreiner/Downloads/logoinverted.png")
WORDMARK = Path("/Users/gianschreiner/Downloads/a07965de-245e-435a-95c4-22677056d1c3.png")
W, H = 640, 480


def font(size, bold=False):
    names = [
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf" if bold else "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/SFNS.ttf",
        "/Library/Fonts/Arial.ttf",
    ]
    for name in names:
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return ImageFont.load_default()


F_TITLE = font(27, True)
F_SUB = font(15, True)
F_TILE = font(17, True)
F_BODY = font(15, True)
F_SMALL = font(12, False)
F_CTRL = font(14, True)


def dark_mask(img):
    gray = img.convert("L")
    return gray.point(lambda p: max(0, min(255, 255 - p)) if p < 245 else 0)


def paste_dark_logo(base, path, box, alpha=255):
    if not path.exists():
        return
    img = Image.open(path).convert("RGB")
    mask = dark_mask(img).filter(ImageFilter.GaussianBlur(0.2))
    img = Image.new("RGBA", img.size, (14, 12, 12, alpha))
    img.putalpha(mask.point(lambda p: int(p * alpha / 255)))
    img.thumbnail((box[2], box[3]), Image.Resampling.LANCZOS)
    x, y = box[0], box[1]
    base.alpha_composite(img, (x, y))


def rounded(draw, xy, radius, fill, outline=None, width=1):
    draw.rounded_rectangle(xy, radius=radius, fill=fill, outline=outline, width=width)


def text_center(draw, box, text, fnt, fill):
    bbox = draw.textbbox((0, 0), text, font=fnt)
    tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
    x = box[0] + (box[2] - box[0] - tw) // 2
    y = box[1] + (box[3] - box[1] - th) // 2
    draw.text((x, y), text, font=fnt, fill=fill)


def icon_usb(draw, cx, cy, color):
    draw.rounded_rectangle((cx - 10, cy - 3, cx + 10, cy + 32), radius=5, fill=color)
    draw.rectangle((cx - 7, cy - 16, cx + 7, cy - 3), fill=color)
    draw.rectangle((cx - 3, cy - 23, cx + 3, cy - 16), fill=color)
    draw.rectangle((cx - 8, cy - 25, cx + 8, cy - 21), fill=color)
    draw.rectangle((cx - 4, cy - 12, cx - 1, cy - 9), fill=(255, 255, 255, 230))
    draw.rectangle((cx + 2, cy - 12, cx + 5, cy - 9), fill=(255, 255, 255, 230))


def icon_grid(draw, cx, cy, color):
    for dx in (-18, 8):
        for dy in (-18, 8):
            draw.rounded_rectangle((cx + dx, cy + dy, cx + dx + 20, cy + dy + 20), radius=5, fill=color)


def icon_download(draw, cx, cy, color):
    draw.polygon([(cx, cy - 30), (cx - 20, cy - 6), (cx - 8, cy - 6), (cx - 8, cy + 16), (cx + 8, cy + 16), (cx + 8, cy - 6), (cx + 20, cy - 6)], fill=color)
    draw.rounded_rectangle((cx - 25, cy + 26, cx + 25, cy + 34), radius=4, fill=color)


def icon_gear(draw, cx, cy, color):
    for i in range(8):
        a = math.pi * 2 * i / 8
        x = cx + math.cos(a) * 24
        y = cy + math.sin(a) * 24
        draw.rounded_rectangle((x - 5, y - 5, x + 5, y + 5), radius=2, fill=color)
    draw.ellipse((cx - 25, cy - 25, cx + 25, cy + 25), fill=color)
    draw.ellipse((cx - 10, cy - 10, cx + 10, cy + 10), fill=(244, 246, 248, 255))


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


def build_screen(connected):
    img = Image.new("RGBA", (W, H), (247, 248, 249, 255))
    d = ImageDraw.Draw(img, "RGBA")

    for y in range(H):
        shade = int(255 - y * 0.025)
        d.line((0, y, W, y), fill=(shade, shade, shade, 255))

    # Wii-like red wave fields.
    d.polygon([(0, 146), (160, 172), (360, 165), (640, 112), (640, 168), (380, 205), (170, 208), (0, 182)], fill=(242, 0, 18, 210))
    d.polygon([(0, 382), (170, 352), (362, 374), (640, 340), (640, 480), (0, 480)], fill=(226, 0, 14, 230))
    d.polygon([(0, 398), (200, 388), (410, 415), (640, 385), (640, 428), (395, 452), (160, 425), (0, 440)], fill=(255, 80, 92, 120))

    # Oversized watermark mascot.
    if MASCOT.exists():
        mark = Image.open(MASCOT).convert("RGB")
        mask = dark_mask(mark).filter(ImageFilter.GaussianBlur(1.0))
        mark_rgba = Image.new("RGBA", mark.size, (220, 0, 15, 62))
        mark_rgba.putalpha(mask.point(lambda p: int(p * 0.18)))
        mark_rgba.thumbnail((305, 305), Image.Resampling.LANCZOS)
        img.alpha_composite(mark_rgba, (350, 12))

    # Header logos.
    paste_dark_logo(img, MASCOT, (32, 32, 88, 88), 255)
    paste_dark_logo(img, WORDMARK, (122, 26, 205, 76), 255)
    d.text((124, 94), "The Spiky Channel", font=F_TITLE, fill=(20, 20, 22, 255))
    d.line((124, 127, 284, 127), fill=(238, 0, 18, 220), width=2)
    d.text((124, 138), "Development Build", font=F_SUB, fill=(238, 0, 18, 255))

    # Status panel.
    rounded(d, (24, 206, 214, 337), 11, (255, 245, 246, 218), (255, 255, 255, 230), 2)
    d.rounded_rectangle((28, 210, 210, 333), radius=10, outline=(205, 0, 12, 32), width=1)
    status_color = (30, 205, 28, 255) if connected else (238, 0, 18, 255)
    status_text = "Connected" if connected else "Not Found"
    d.rectangle((47, 239, 59, 260), fill=(238, 0, 18, 255))
    d.rectangle((50, 232, 56, 239), fill=(238, 0, 18, 255))
    d.text((72, 237), f"USB: {status_text}", font=F_BODY, fill=(18, 18, 20, 255))
    d.ellipse((192, 240, 206, 254), fill=status_color)
    d.line((38, 272, 200, 272), fill=(200, 0, 20, 38), width=1)
    d.ellipse((43, 292, 62, 311), fill=(238, 0, 18, 255))
    d.ellipse((50, 299, 55, 304), fill=(255, 245, 246, 255))
    d.text((72, 292), "Core: Ready", font=F_BODY, fill=(18, 18, 20, 255))
    d.ellipse((192, 294, 206, 308), fill=(30, 205, 28, 255))

    tiles = [
        ("USB Core", "usb", True),
        ("Apps", "grid", False),
        ("Downloads", "down", False),
        ("Settings", "gear", False),
    ]
    x0 = 220
    for i, (label, kind, selected) in enumerate(tiles):
        x = x0 + i * 104
        box = (x, 188, x + 90, 346)
        if selected:
            shadow = Image.new("RGBA", (W, H), (0, 0, 0, 0))
            sd = ImageDraw.Draw(shadow, "RGBA")
            sd.rounded_rectangle((box[0] - 6, box[1] - 6, box[2] + 6, box[3] + 6), radius=14, fill=(238, 0, 18, 120))
            img.alpha_composite(shadow.filter(ImageFilter.GaussianBlur(9)))
            rounded(d, box, 11, (210, 0, 14, 242), (255, 255, 255, 255), 3)
            d.arc((x + 10, 204, x + 70, 285), 200, 310, fill=(255, 82, 92, 72), width=16)
            icon_col = (255, 244, 244, 255)
            text_col = (255, 255, 255, 255)
        else:
            rounded(d, box, 9, (255, 255, 255, 220), (215, 218, 222, 255), 2)
            d.polygon([(x + 12, 340), (x + 68, 202), (x + 90, 202), (x + 34, 340)], fill=(238, 0, 18, 24))
            icon_col = (18, 18, 20, 255)
            text_col = (18, 18, 20, 255)
        cx = x + 45
        if kind == "usb":
            icon_usb(d, cx, 232, icon_col)
        elif kind == "grid":
            icon_grid(d, cx, 238, icon_col)
        elif kind == "down":
            icon_download(d, cx, 238, icon_col)
        else:
            icon_gear(d, cx, 240, icon_col)
        text_center(d, (x, 294, x + 90, 334), label, F_TILE, text_col)

    # Control bar.
    rounded(d, (18, 412, 622, 460), 11, (255, 238, 240, 230), (255, 255, 255, 245), 2)
    for x in (232, 412):
        d.line((x, 421, x, 451), fill=(100, 92, 96, 85), width=1)
    controls = [(95, "A", "Select"), (310, "B", "Back"), (500, "", "HOME  Exit")]
    for x, key, label in controls:
        d.ellipse((x - 17, 424, x + 17, 458), fill=(234, 238, 242, 255), outline=(100, 106, 112, 190), width=2)
        if key:
            text_center(d, (x - 17, 424, x + 17, 458), key, F_CTRL, (40, 42, 45, 255))
        else:
            d.polygon([(x, 431), (x - 10, 441), (x + 10, 441)], fill=(40, 42, 45, 255))
            d.rectangle((x - 7, 441, x + 7, 452), fill=(40, 42, 45, 255))
            d.rectangle((x - 2, 446, x + 3, 452), fill=(234, 238, 242, 255))
        d.text((x + 27, 432), label, font=F_CTRL, fill=(36, 36, 38, 255))

    return img.convert("RGB")


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    connected = build_screen(True)
    disconnected = build_screen(False)
    connected.save(OUT / "ui_connected_preview.png")
    disconnected.save(OUT / "ui_disconnected_preview.png")
    header = [
        "/* Generated by scripts/generate_red_ui.py. */",
        "#pragma once",
        "#include <gccore.h>",
        "#define SPIKY_UI_WIDTH 640",
        "#define SPIKY_UI_HEIGHT 480",
        image_to_c_array(connected, "spiky_ui_connected"),
        image_to_c_array(disconnected, "spiky_ui_disconnected"),
        "",
    ]
    (ROOT / "source" / "ui_background.h").write_text("\n\n".join(header))


if __name__ == "__main__":
    main()
