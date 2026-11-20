#!/usr/bin/env python3
import math
import os
import random
import sys

import numpy as np
from PIL import Image

SIZE = 512
FACES = ["px", "nx", "py", "ny", "pz", "nz"]

STYLES = {
    "city": {
        "sky_top": (0.30, 0.50, 0.82), "sky_horizon": (0.78, 0.84, 0.90),
        "ground": (0.30, 0.30, 0.31), "building": (0.42, 0.47, 0.55), "building_var": 0.10,
        "window_lit": (0.75, 0.82, 0.90), "window_dark": (0.28, 0.33, 0.40), "lit_chance": 0.25,
        "stars": 0, "haze": (0.80, 0.85, 0.90),
    },
    "night": {
        "sky_top": (0.01, 0.015, 0.05), "sky_horizon": (0.10, 0.08, 0.16),
        "ground": (0.02, 0.02, 0.025), "building": (0.035, 0.04, 0.06), "building_var": 0.02,
        "window_lit": (1.00, 0.78, 0.38), "window_dark": (0.03, 0.035, 0.05), "lit_chance": 0.35,
        "stars": 400, "haze": (0.14, 0.10, 0.18),
    },
}


def face_directions(face):
    t, s = np.meshgrid((np.arange(SIZE) + 0.5) / SIZE * 2 - 1, (np.arange(SIZE) + 0.5) / SIZE * 2 - 1, indexing="ij")
    one = np.ones_like(s)
    d = {
        "px": (one, -t, -s), "nx": (-one, -t, s),
        "py": (s, one, t), "ny": (s, -one, -t),
        "pz": (s, -t, one), "nz": (-s, -t, -one),
    }[face]
    v = np.stack(d, axis=-1)
    return v / np.linalg.norm(v, axis=-1, keepdims=True)


def skyline(rng):
    buildings = []
    az = 0.0
    while az < 360.0:
        width = rng.uniform(2.0, 7.0)
        height = rng.uniform(3.0, 16.0) if rng.random() < 0.85 else rng.uniform(16.0, 26.0)
        tone = rng.uniform(-1.0, 1.0)
        buildings.append((az, az + width, height, tone, rng.randrange(1 << 30)))
        az += width + rng.uniform(0.0, 1.0)
    return buildings


def render(style_name, out_dir, seed=7):
    st = STYLES[style_name]
    rng = random.Random(seed)
    city = skyline(rng)
    starts = np.array([b[0] for b in city])
    stars = [(rng.uniform(0, 360), rng.uniform(5, 85), rng.uniform(0.5, 1.0)) for _ in range(st["stars"])]
    os.makedirs(out_dir, exist_ok=True)

    for face in FACES:
        d = face_directions(face)
        elev = np.degrees(np.arcsin(np.clip(d[..., 1], -1, 1)))
        azim = (np.degrees(np.arctan2(d[..., 0], d[..., 2])) + 360.0) % 360.0

        k = np.clip(elev / 60.0, 0, 1)[..., None] ** 0.6
        img = np.array(st["sky_horizon"]) * (1 - k) + np.array(st["sky_top"]) * k
        haze = np.exp(-np.clip(elev, 0, None) / 4.0)[..., None]
        img = img * (1 - haze * 0.5) + np.array(st["haze"]) * haze * 0.5

        for az, alt, brightness in stars:
            dist = np.hypot((azim - az + 180) % 360 - 180, elev - alt)
            img = np.where((dist < 0.25)[..., None], np.array([brightness] * 3), img)

        idx = np.clip(np.searchsorted(starts, azim, side="right") - 1, 0, len(city) - 1)
        for i in np.unique(idx):
            a0, a1, h, tone, bseed = city[i]
            mask = (idx == i) & (azim >= a0) & (azim < a1) & (elev < h) & (elev > -1.0)
            if not mask.any():
                continue
            base = np.clip(np.array(st["building"]) * (1 + tone * st["building_var"] * 4), 0, 1)
            col = (azim - a0) / 0.55
            row = (h - elev) / 0.75
            in_window = ((col % 1.0) > 0.35) & ((row % 1.0) > 0.40) & (row > 0.6)
            cell = (np.floor(col).astype(np.int64) * 7919 + np.floor(row).astype(np.int64) * 104729 + bseed) % 1000
            lit = cell < st["lit_chance"] * 1000
            window = np.where(lit[..., None], np.array(st["window_lit"]), np.array(st["window_dark"]))
            colour = np.where(in_window[..., None], window, base)
            img = np.where(mask[..., None], colour, img)

        img = np.where((elev < -1.0)[..., None], np.array(st["ground"]), img)
        srgb = np.clip(img, 0, 1) ** (1 / 2.2)
        Image.fromarray((srgb * 255).astype(np.uint8)).save(os.path.join(out_dir, face + ".png"), optimize=True)
    print("wrote", out_dir)


if __name__ == "__main__":
    root = sys.argv[1] if len(sys.argv) > 1 else "assets/skybox"
    render("city", os.path.join(root, "city"))
    render("night", os.path.join(root, "night"))
