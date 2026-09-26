#!/usr/bin/env python3
"""Generate the procedural textures used by CSS171_PE1_Soriano.cpp.

Every texture is drawn from a fixed random seed, so re-running this script
reproduces the committed PNGs exactly. All textures except the banner tile
seamlessly: patterns repeat on the image period and noise is wrapped before
blurring, so GL_REPEAT sampling shows no seams.

Usage: python3 tools/generate_textures.py [output_dir]   (default: textures)
Requires Pillow.
"""

import random
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

SIZE = 256  # edge length of every tiling texture, in pixels


def clamp(value):
    return max(0, min(255, int(value)))


def shade(color, factor):
    """Scale an RGB color by factor, clamped to the valid range."""
    return tuple(clamp(c * factor) for c in color)


def add_noise(image, rng, amplitude):
    """Add independent per-pixel brightness noise (tiles trivially)."""
    pixels = image.load()
    width, height = image.size
    for y in range(height):
        for x in range(width):
            offset = rng.uniform(-amplitude, amplitude)
            r, g, b = pixels[x, y]
            pixels[x, y] = (clamp(r + offset), clamp(g + offset), clamp(b + offset))


def tileable_blur(image, radius):
    """Blur with wrap-around edges so the result still tiles seamlessly."""
    width, height = image.size
    tiled = Image.new("RGB", (width * 3, height * 3))
    for ty in range(3):
        for tx in range(3):
            tiled.paste(image, (tx * width, ty * height))
    blurred = tiled.filter(ImageFilter.GaussianBlur(radius))
    return blurred.crop((width, height, width * 2, height * 2))


def stone_texture(rng):
    """Staggered ashlar blocks with light mortar joints."""
    mortar = (150, 142, 128)
    image = Image.new("RGB", (SIZE, SIZE), mortar)
    draw = ImageDraw.Draw(image)
    row_height, block_width, joint = 64, 128, 4
    for row in range(SIZE // row_height):
        stagger = (block_width // 2) * (row % 2)
        # Draw one extra block so the staggered row wraps across the edge
        for column in range(-1, SIZE // block_width + 1):
            left = column * block_width + stagger
            base = rng.randint(105, 140)
            color = (base, base - 3, base - 8)
            draw.rectangle(
                (left + joint, row * row_height + joint,
                 left + block_width - 1, row * row_height + row_height - 1),
                fill=color,
            )
    add_noise(image, rng, 14)
    return image


def roof_texture(rng):
    """Rows of staggered terracotta shingles with shadowed lower edges."""
    image = Image.new("RGB", (SIZE, SIZE), (70, 25, 18))
    draw = ImageDraw.Draw(image)
    row_height, shingle_width, gap = 32, 32, 2
    for row in range(SIZE // row_height):
        stagger = (shingle_width // 2) * (row % 2)
        top = row * row_height
        for column in range(-1, SIZE // shingle_width + 1):
            left = column * shingle_width + stagger
            base = (rng.randint(160, 190), rng.randint(62, 80), rng.randint(40, 52))
            draw.rectangle((left + gap, top, left + shingle_width - 1, top + row_height - 1), fill=base)
            # Darker band at the bottom of each shingle reads as overlap shadow
            draw.rectangle(
                (left + gap, top + row_height - 7, left + shingle_width - 1, top + row_height - 1),
                fill=shade(base, 0.62),
            )
    add_noise(image, rng, 10)
    return image


def wood_texture(rng):
    """Vertical oak planks with grain streaks and two iron straps."""
    image = Image.new("RGB", (SIZE, SIZE))
    pixels = image.load()
    plank_width = 32
    plank_tones = [rng.uniform(0.85, 1.12) for _ in range(SIZE // plank_width)]
    # Grain: a random brightness per column, blurred vertically into streaks
    grain = [rng.uniform(-18, 18) for _ in range(SIZE)]
    for y in range(SIZE):
        for x in range(SIZE):
            tone = plank_tones[x // plank_width]
            base = (112 * tone + grain[x], 72 * tone + grain[x] * 0.7, 40 * tone + grain[x] * 0.4)
            if x % plank_width in (0, 1):  # dark gap between planks
                base = (38, 24, 14)
            pixels[x, y] = tuple(clamp(c) for c in base)
    add_noise(image, rng, 8)
    draw = ImageDraw.Draw(image)
    for strap_top in (40, 196):
        draw.rectangle((0, strap_top, SIZE - 1, strap_top + 14), fill=(52, 52, 56))
        for rivet_x in range(plank_width // 2, SIZE, plank_width):
            draw.ellipse((rivet_x - 3, strap_top + 4, rivet_x + 3, strap_top + 10), fill=(120, 120, 126))
    return image


def grass_texture(rng):
    """Mottled meadow green: blurred color blotches plus fine noise."""
    blotches = Image.new("RGB", (SIZE, SIZE))
    pixels = blotches.load()
    for y in range(SIZE):
        for x in range(SIZE):
            pixels[x, y] = (rng.randint(55, 85), rng.randint(115, 155), rng.randint(40, 62))
    image = tileable_blur(blotches, 3)
    add_noise(image, rng, 12)
    return image


def dirt_texture(rng):
    """Packed earth path with scattered pebbles."""
    base = Image.new("RGB", (SIZE, SIZE))
    pixels = base.load()
    for y in range(SIZE):
        for x in range(SIZE):
            pixels[x, y] = (rng.randint(120, 150), rng.randint(95, 118), rng.randint(62, 80))
    image = tileable_blur(base, 2)
    draw = ImageDraw.Draw(image)
    for _ in range(90):
        x, y = rng.randrange(8, SIZE - 8), rng.randrange(8, SIZE - 8)
        radius = rng.randint(2, 5)
        grey = rng.randint(130, 175)
        draw.ellipse((x - radius, y - radius, x + radius, y + radius), fill=(grey, grey - 6, grey - 14))
    add_noise(image, rng, 8)
    return image


def banner_texture():
    """Royal banner: crimson field, gold border, gold diagonal band.

    Not tiled; the banner shows the whole image once (sampled with clamping).
    """
    width, height = 128, 80
    image = Image.new("RGB", (width, height), (170, 22, 34))
    draw = ImageDraw.Draw(image)
    gold = (236, 188, 60)
    draw.rectangle((0, 0, width - 1, height - 1), outline=gold, width=5)
    draw.polygon([(0, 16), (16, 0), (width, height - 16), (width - 16, height)], fill=gold)
    return image


def main():
    output_dir = Path(sys.argv[1] if len(sys.argv) > 1 else "textures")
    output_dir.mkdir(parents=True, exist_ok=True)

    # One seeded generator per texture keeps each file stable even if
    # another texture's recipe changes.
    textures = {
        "stone.png": lambda: stone_texture(random.Random(171_01)),
        "roof.png": lambda: roof_texture(random.Random(171_02)),
        "wood.png": lambda: wood_texture(random.Random(171_03)),
        "grass.png": lambda: grass_texture(random.Random(171_04)),
        "dirt.png": lambda: dirt_texture(random.Random(171_05)),
        "banner.png": banner_texture,
    }
    for name, build in textures.items():
        path = output_dir / name
        build().save(path, optimize=True)
        print(f"wrote {path}")


if __name__ == "__main__":
    main()
