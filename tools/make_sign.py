#!/usr/bin/env python3
import sys

from PIL import Image, ImageDraw, ImageFont

FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
SMALL = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"


def main(out_path, title="LEGENDS SHOWROOM", subtitle="classic and modern icons"):
    w, h = 1024, 256
    img = Image.new("RGB", (w, h), (0, 0, 0))
    draw = ImageDraw.Draw(img)
    big = ImageFont.truetype(FONT, 76)
    small = ImageFont.truetype(SMALL, 34)
    tw = draw.textlength(title, font=big)
    draw.text(((w - tw) / 2, 50), title, font=big, fill=(255, 236, 190))
    sw = draw.textlength(subtitle, font=small)
    draw.text(((w - sw) / 2, 168), subtitle, font=small, fill=(200, 190, 170))
    draw.rectangle([60, 150, w - 60, 153], fill=(220, 190, 120))
    img.save(out_path, optimize=True)
    print("wrote", out_path)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "assets/textures/sign.png")
