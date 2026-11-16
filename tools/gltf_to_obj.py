#!/usr/bin/env python3
import fnmatch
import json
import os
import re
import shutil
import sys

import numpy as np
from PIL import Image

CARS = {
    "porsche930": {
        "front": "+z", "length": 4.2,
        "paint": ["paint"],
        "wheels": ["930_rim", "930_tire"],
        "drop": ["coat"],
        "tint": [],
        "shadow": "material_0",
        "max_texture": {"930_plastics": 2048},
        "phong": {
            "930_plastics": (24, 0.25, None), "930_stickers": (32, 0.3, None), "plate": (32, 0.3, None),
            "930_rim": (64, 0.8, None), "930_tire": (6, 0.08, None), "930_lights": (128, 1.0, None),
            "930_lights_refraction": (128, 1.0, 0.25), "glass": (128, 1.0, 0.35),
            "930_chromes": (128, 1.0, None), "black": (16, 0.2, None), "930_wunderbaum": (8, 0.1, None),
            "material_0": (1, 0.0, None),
        },
    },
    "supra93": {
        "front": "+z", "length": 4.43,
        "paint": ["Body_Paint_-_White"],
        "wheels": ["Tyre", "material", "Brake_Disc"],
        "drop": ["Interior_-_Plastic", "Leather", "Leather_-_Dark", "Dashboard", "Carpet_2",
                 "Seering_Wheel", "Seat_belt", "Gear_shifter*", "Interior_Detail*", "Speaker",
                 "Window_Glass_-_Gauge", "Fuel_Alarm", "Defrost", "Radiator"],
        "tint": ["Window_Glass", "Window_Glass_-_White", "Window_Glass_not_transparent"],
        "phong": {"Tyre": (6, 0.08, None), "material": (64, 0.8, None), "Metal_-_Chrome": (128, 1.0, None),
                  "Lights": (128, 1.0, None), "Rubber": (8, 0.1, None)},
    },
    "r34": {
        "front": "+z", "length": 4.5,
        "paint": ["*Paint_Material1"],
        "wheels": ["*_Wheel1A_*", "riminside", "material"],
        "drop": ["*InteriorA_Material1*", "*InteriorTillingA*", "*EngineA_Material1", "seat"],
        "tint": ["*Window_Material1*"],
        "phong": {"*Wheel1A_Material1.013": (64, 0.8, None), "riminside": (16, 0.2, None)},
    },
    "aventador": {
        "front": "+x", "length": 4.7,
        "paint": ["RED_PAINT"],
        "wheels": ["*_Wheel2A_*"],
        "drop": ["*InteriorA_Material", "*EngineA_Material", "*SeatBelt_Material"],
        "tint": ["*Window_Material"],
        "phong": {"*Carbon1_Material": (48, 0.5, None), "PLASTIC": (16, 0.2, None)},
    },
}

TINT_COLOR = (0.02, 0.025, 0.03)
TINT_ALPHA = 0.75

COMPONENT = {5126: np.float32, 5125: np.uint32, 5123: np.uint16, 5121: np.uint8}
COUNT = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}

class Gltf:
    def __init__(self, path):
        self.dir = os.path.dirname(os.path.abspath(path))
        with open(path) as f:
            self.g = json.load(f)
        with open(os.path.join(self.dir, self.g["buffers"][0]["uri"]), "rb") as f:
            self.buf = f.read()

    def accessor(self, i):
        a = self.g["accessors"][i]
        bv = self.g["bufferViews"][a["bufferView"]]
        dt = np.dtype(COMPONENT[a["componentType"]])
        n = COUNT[a["type"]]
        offset = bv.get("byteOffset", 0) + a.get("byteOffset", 0)
        stride = bv.get("byteStride", 0) or dt.itemsize * n
        raw = np.frombuffer(self.buf, np.uint8, count=stride * (a["count"] - 1) + dt.itemsize * n, offset=offset)
        rows = np.lib.stride_tricks.as_strided(raw, (a["count"], dt.itemsize * n), (stride, 1)).copy()
        data = rows.view(dt).reshape(a["count"], n)
        return data[:, 0] if n == 1 else data

    @staticmethod
    def local_matrix(node):
        if "matrix" in node:
            return np.array(node["matrix"], float).reshape(4, 4).T
        t = np.eye(4)
        t[:3, 3] = node.get("translation", [0, 0, 0])
        x, y, z, w = node.get("rotation", [0, 0, 0, 1])
        r = np.eye(4)
        r[:3, :3] = [[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
                     [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
                     [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]]
        s = np.diag(list(node.get("scale", [1, 1, 1])) + [1])
        return t @ r @ s

    def primitives(self):
        out = []

        def walk(i, parent_m):
            node = self.g["nodes"][i]
            m = parent_m @ self.local_matrix(node)
            if "mesh" in node:
                normal_m = np.linalg.inv(m[:3, :3]).T
                for p in self.g["meshes"][node["mesh"]]["primitives"]:
                    at = p["attributes"]
                    pos = self.accessor(at["POSITION"]).astype(float)
                    nrm = self.accessor(at["NORMAL"]).astype(float) if "NORMAL" in at else np.tile([0, 1.0, 0], (len(pos), 1))
                    uv = self.accessor(at["TEXCOORD_0"]).astype(float) if "TEXCOORD_0" in at else np.zeros((len(pos), 2))
                    tri = self.accessor(p["indices"]).astype(np.int64).reshape(-1, 3)
                    if np.linalg.det(m[:3, :3]) < 0:
                        tri = tri[:, [0, 2, 1]]
                    nrm = (normal_m @ nrm.T).T
                    nrm /= np.linalg.norm(nrm, axis=1, keepdims=True) + 1e-12
                    out.append({"mat": self.g["materials"][p["material"]]["name"],
                                "pos": (m[:3, :3] @ pos.T).T + m[:3, 3], "nrm": nrm, "uv": uv, "tri": tri})
            for c in node.get("children", []):
                walk(c, m)

        for root in self.g["scenes"][0]["nodes"]:
            walk(root, np.eye(4))
        return out

def matches(name, patterns):
    return any(fnmatch.fnmatchcase(name, p) for p in patterns)

def safe(name):
    return re.sub(r"[^A-Za-z0-9_.-]", "_", name)

def sub_primitive(p, tri_mask):
    tri = p["tri"][tri_mask]
    used, inverse = np.unique(tri, return_inverse=True)
    return {"mat": p["mat"], "pos": p["pos"][used], "nrm": p["nrm"][used], "uv": p["uv"][used],
            "tri": inverse.reshape(-1, 3)}

def main(car_id, src, out_dir):
    cfg = CARS[car_id]
    gl = Gltf(src)
    prims = [p for p in gl.primitives() if not matches(p["mat"], cfg["drop"]) and len(p["tri"])]

    rot = np.eye(3)
    if cfg["front"] == "+z":
        rot = np.array([[0, 0, 1], [0, 1, 0], [-1, 0, 0]], float)
    for p in prims:
        p["pos"] = p["pos"] @ rot.T
        p["nrm"] = p["nrm"] @ rot.T

    shadow = cfg.get("shadow")
    car = np.vstack([p["pos"] for p in prims if p["mat"] != shadow])
    lo, hi = car.min(0), car.max(0)
    scale = cfg["length"] / (hi[0] - lo[0])
    wheel_pts = np.vstack([p["pos"] for p in prims if matches(p["mat"], cfg["wheels"])])
    offset = np.array([-(lo[0] + hi[0]) / 2, -wheel_pts[:, 1].min(), -(lo[2] + hi[2]) / 2])
    for p in prims:
        p["pos"] = (p["pos"] + offset) * scale

    objects = {}
    for p in prims:
        if p["mat"] == shadow:
            objects.setdefault("shadow", []).append(p)
        elif matches(p["mat"], cfg["wheels"]):
            centre = p["pos"][p["tri"]].mean(axis=1)
            front, right = centre[:, 0] > 0, centre[:, 2] > 0
            for f in (True, False):
                for r in (True, False):
                    mask = (front == f) & (right == r)
                    if mask.any():
                        name = "wheel_" + ("F" if f else "R") + ("R" if r else "L")
                        objects.setdefault(name, []).append(sub_primitive(p, mask))
        else:
            objects.setdefault("body", []).append(p)

    for parts in objects.values():
        for p in parts:
            p["out"] = "paint" if matches(p["mat"], cfg["paint"]) else safe(p["mat"])

    os.makedirs(os.path.join(out_dir, "textures"), exist_ok=True)
    write_mtl(gl, cfg, out_dir, objects)
    write_obj(out_dir, objects, gl)
    lic = os.path.join(gl.dir, "license.txt")
    if os.path.exists(lic):
        shutil.copy(lic, os.path.join(out_dir, "license.txt"))

    total = sum(len(p["tri"]) for parts in objects.values() for p in parts)
    print(f"{car_id}: objects {sorted(objects)}  triangles {total}  scale {scale:.4f}")

def phong_for(cfg, name, pbr):
    for pattern, values in cfg.get("phong", {}).items():
        if fnmatch.fnmatchcase(name, pattern):
            return values
    if matches(name, cfg["paint"]):
        return (128, 1.0, None)
    rough = pbr.get("roughnessFactor", 1.0)
    metal = pbr.get("metallicFactor", 0.0)
    ns = float(np.clip(2.0 / max(rough, 0.05) ** 4 - 2.0, 4, 128))
    ks = float(np.clip(max(0.05, 1.0 - rough), 0.05, 1.0))
    if metal > 0.5:
        ks = max(ks, 0.6)
    return (round(ns), round(ks, 2), None)

def write_mtl(gl, cfg, out_dir, objects):
    by_name = {m["name"]: m for m in gl.g["materials"]}
    wanted = {}
    for parts in objects.values():
        for p in parts:
            wanted.setdefault(p["out"], p["mat"])

    lines = [f"# Converted from glTF by tools/gltf_to_obj.py", ""]
    for out, src in sorted(wanted.items()):
        m = by_name[src]
        pbr = m.get("pbrMetallicRoughness", {})
        r, g, b, a = pbr.get("baseColorFactor", [1, 1, 1, 1])
        ns, ks, alpha = phong_for(cfg, src, pbr)
        if alpha is None:
            alpha = a if m.get("alphaMode") == "BLEND" else 1.0
        if matches(src, cfg["tint"]):
            r, g, b = TINT_COLOR
            alpha = TINT_ALPHA
        lines += [f"newmtl {out}", f"Kd {r:.4f} {g:.4f} {b:.4f}", f"Ks {ks} {ks} {ks}", f"Ns {ns}", f"d {alpha:.3f}"]
        if m.get("doubleSided"):
            lines.append("double_sided 1")
        if m.get("alphaMode") == "BLEND" or alpha < 1.0:
            lines.append("blend 1")
        if "baseColorTexture" in pbr and not matches(src, cfg["tint"]):
            img = gl.g["images"][gl.g["textures"][pbr["baseColorTexture"]["index"]]["source"]]
            dst = f"textures/{out}.png"
            im = Image.open(os.path.join(gl.dir, img["uri"]))
            limit = cfg.get("max_texture", {}).get(src, 1024)
            if max(im.size) > limit:
                f = limit / max(im.size)
                im = im.resize((max(1, int(im.width * f)), max(1, int(im.height * f))), Image.LANCZOS)
            im.save(os.path.join(out_dir, dst), optimize=True)
            lines.append(f"map_Kd {dst}")
        lines.append("")
    with open(os.path.join(out_dir, "car.mtl"), "w") as f:
        f.write("\n".join(lines))

def write_obj(out_dir, objects, gl):
    extras = gl.g.get("asset", {}).get("extras", {})
    with open(os.path.join(out_dir, "car.obj"), "w") as f:
        f.write(f"# {extras.get('title', 'car model')} by {extras.get('author', 'unknown')}, {extras.get('license', '')}\n")
        f.write("# converted by tools/gltf_to_obj.py\nmtllib car.mtl\n")
        base = 1
        for name in sorted(objects):
            f.write(f"o {name}\n")
            for p in sorted(objects[name], key=lambda q: q["out"]):
                n = len(p["pos"])
                f.write("".join(f"v {x:.5f} {y:.5f} {z:.5f}\n" for x, y, z in p["pos"]))
                f.write("".join(f"vt {u:.5f} {1 - v:.5f}\n" for u, v in p["uv"]))
                f.write("".join(f"vn {x:.4f} {y:.4f} {z:.4f}\n" for x, y, z in p["nrm"]))
                f.write(f"usemtl {p['out']}\n")
                t = p["tri"] + base
                f.write("".join(f"f {a}/{a}/{a} {b}/{b}/{b} {c}/{c}/{c}\n" for a, b, c in t))
                base += n

if __name__ == "__main__":
    if len(sys.argv) not in (3, 4) or sys.argv[1] not in CARS:
        sys.exit("usage: python3 tools/gltf_to_obj.py <car id> <scene.gltf> [output dir]\nKnown cars: " + ", ".join(CARS))
    car_id, src = sys.argv[1], sys.argv[2]
    main(car_id, src, sys.argv[3] if len(sys.argv) == 4 else os.path.join("assets", "models", car_id))
