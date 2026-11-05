#!/usr/bin/env python3
"""
Converts the Sketchfab glTF export of "FREE 1975 Porsche 911 (930) Turbo"
(Lionsharp Studios, CC BY 4.0) into one OBJ + MTL that our C++ loader reads.

What it does:
  * bakes every glTF node transform into world-space vertices
  * drops the "coat" clear-coat shell (43k triangles, not needed for Blinn-Phong)
  * rotates the car so it faces +X, puts the tyres on y = 0, centres it and
    scales it to TARGET_LENGTH units
  * writes objects: body, wheel_FL/FR/RL/RR (rim + tyre), shadow (ground plane)
  * writes only baseColor textures, downscaled (we do not use PBR maps)

Usage:  python3 tools/gltf_to_obj.py <path/to/scene.gltf> assets/models/porsche930
Needs:  numpy, Pillow   (pip install numpy pillow)
"""
import json
import os
import shutil
import sys

import numpy as np
from PIL import Image

TARGET_LENGTH = 4.2                       # car length in scene units
DROP_MATERIALS = {"coat"}                 # clear-coat shell
WHEEL_MATERIALS = {"930_rim", "930_tire"}
SHADOW_MATERIAL = "material_0"            # baked contact-shadow plane
MAX_TEXTURE = {"930_plastics": 2048}      # default max size is 1024

# Blinn-Phong parameters chosen by hand (glTF stores PBR values instead).
# name: (shininess Ns, specular Ks, alpha d or None to keep glTF value)
PHONG = {
    "paint":                 (128, 1.0, None),
    "930_plastics":          (24, 0.25, None),
    "930_stickers":          (32, 0.3, None),
    "plate":                 (32, 0.3, None),
    "930_rim":               (64, 0.8, None),
    "930_tire":              (6, 0.08, None),
    "930_lights":            (128, 1.0, None),
    "930_lights_refraction": (128, 1.0, 0.25),
    "glass":                 (128, 1.0, 0.35),
    "930_chromes":           (128, 1.0, None),
    "black":                 (16, 0.2, None),
    "930_wunderbaum":        (8, 0.1, None),
    SHADOW_MATERIAL:         (1, 0.0, None),
}

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
        """List of dicts: owner node name, material, world positions/normals, uv, triangles."""
        out = []

        def walk(i, parent_m, owner):
            node = self.g["nodes"][i]
            m = parent_m @ self.local_matrix(node)
            if "mesh" in node:
                normal_m = np.linalg.inv(m[:3, :3]).T
                for p in self.g["meshes"][node["mesh"]]["primitives"]:
                    at = p["attributes"]
                    pos = self.accessor(at["POSITION"]).astype(float)
                    nrm = self.accessor(at["NORMAL"]).astype(float)
                    uv = self.accessor(at["TEXCOORD_0"]).astype(float) if "TEXCOORD_0" in at else np.zeros((len(pos), 2))
                    tri = self.accessor(p["indices"]).astype(np.int64).reshape(-1, 3)
                    if np.linalg.det(m[:3, :3]) < 0:
                        tri = tri[:, [0, 2, 1]]
                    nrm = (normal_m @ nrm.T).T
                    nrm /= np.linalg.norm(nrm, axis=1, keepdims=True) + 1e-12
                    out.append({"owner": owner, "mat": self.g["materials"][p["material"]]["name"],
                                "pos": (m[:3, :3] @ pos.T).T + m[:3, 3], "nrm": nrm, "uv": uv, "tri": tri})
            for c in node.get("children", []):
                walk(c, m, owner if "mesh" in node else node.get("name", ""))

        for root in self.g["scenes"][0]["nodes"]:
            walk(root, np.eye(4), "")
        return out


def main(src, out_dir):
    gl = Gltf(src)
    prims = [p for p in gl.primitives() if p["mat"] not in DROP_MATERIALS]

    # Rotate +90 deg about Y so the car's front (+Z in the source) faces +X.
    rot = np.array([[0, 0, 1], [0, 1, 0], [-1, 0, 0]], float)
    for p in prims:
        p["pos"] = p["pos"] @ rot.T
        p["nrm"] = p["nrm"] @ rot.T

    car = np.vstack([p["pos"] for p in prims if p["mat"] != SHADOW_MATERIAL])
    lo, hi = car.min(0), car.max(0)
    scale = TARGET_LENGTH / (hi[0] - lo[0])
    tyres = np.vstack([p["pos"] for p in prims if p["mat"] == "930_tire"])
    offset = np.array([-(lo[0] + hi[0]) / 2, -tyres[:, 1].min(), -(lo[2] + hi[2]) / 2])
    for p in prims:
        p["pos"] = (p["pos"] + offset) * scale

    # Group wheel parts: the 4 big rim+tyre owners are wheels, tiny pieces join the nearest one.
    wheel_owners = {}
    for p in prims:
        if p["mat"] in WHEEL_MATERIALS:
            wheel_owners.setdefault(p["owner"], []).append(p)
    big = {o: ps for o, ps in wheel_owners.items() if sum(len(q["tri"]) for q in ps) > 1000}
    centres = {o: np.vstack([q["pos"] for q in ps]).mean(0) for o, ps in big.items()}
    wheel_of = {}
    for o, ps in wheel_owners.items():
        c = np.vstack([q["pos"] for q in ps]).mean(0)
        wheel_of[o] = min(centres, key=lambda k: np.linalg.norm(centres[k] - c))
    labels = {}
    for o, c in centres.items():          # front = +X, right side = +Z
        labels[o] = "wheel_" + ("F" if c[0] > 0 else "R") + ("R" if c[2] > 0 else "L")

    objects = {}
    for p in prims:
        if p["owner"] in wheel_of and p["mat"] in WHEEL_MATERIALS:
            name = labels[wheel_of[p["owner"]]]
        elif p["mat"] == SHADOW_MATERIAL:
            name = "shadow"
        else:
            name = "body"
        objects.setdefault(name, {}).setdefault(p["mat"], []).append(p)

    os.makedirs(os.path.join(out_dir, "textures"), exist_ok=True)
    write_mtl(gl, out_dir, {p["mat"] for p in prims})
    write_obj(out_dir, objects)
    lic = os.path.join(gl.dir, "license.txt")
    if os.path.exists(lic):
        shutil.copy(lic, os.path.join(out_dir, "license.txt"))

    total = sum(len(p["tri"]) for p in prims)
    print(f"objects: {sorted(objects)}  triangles: {total}  scale: {scale:.3f}")


def write_mtl(gl, out_dir, used):
    lines = ["# Converted from glTF by tools/gltf_to_obj.py", ""]
    for m in gl.g["materials"]:
        name = m["name"]
        if name not in used:
            continue
        pbr = m.get("pbrMetallicRoughness", {})
        r, g, b, a = pbr.get("baseColorFactor", [1, 1, 1, 1])
        ns, ks, alpha = PHONG.get(name, (32, 0.5, None))
        if alpha is None:
            alpha = a if m.get("alphaMode") == "BLEND" else 1.0
        lines += [f"newmtl {name}", f"Kd {r:.4f} {g:.4f} {b:.4f}", f"Ks {ks} {ks} {ks}", f"Ns {ns}", f"d {alpha:.3f}"]
        if m.get("doubleSided"):
            lines.append("double_sided 1")
        if m.get("alphaMode") == "BLEND" or alpha < 1.0:
            lines.append("blend 1")           # drawn in the transparent pass
        if "baseColorTexture" in pbr:
            img = gl.g["images"][gl.g["textures"][pbr["baseColorTexture"]["index"]]["source"]]
            src = os.path.join(gl.dir, img["uri"])
            dst = f"textures/{name}.png"
            im = Image.open(src)
            limit = MAX_TEXTURE.get(name, 1024)
            if max(im.size) > limit:
                f = limit / max(im.size)
                im = im.resize((max(1, int(im.width * f)), max(1, int(im.height * f))), Image.LANCZOS)
            im.save(os.path.join(out_dir, dst), optimize=True)
            lines.append(f"map_Kd {dst}")
        lines.append("")
    with open(os.path.join(out_dir, "car.mtl"), "w") as f:
        f.write("\n".join(lines))


def write_obj(out_dir, objects):
    with open(os.path.join(out_dir, "car.obj"), "w") as f:
        f.write("# FREE 1975 Porsche 911 (930) Turbo by Lionsharp Studios, CC BY 4.0\n")
        f.write("# converted by tools/gltf_to_obj.py\nmtllib car.mtl\n")
        base = 1
        for name in sorted(objects):
            f.write(f"o {name}\n")
            for mat, parts in objects[name].items():
                for p in parts:
                    n = len(p["pos"])
                    f.write("".join(f"v {x:.5f} {y:.5f} {z:.5f}\n" for x, y, z in p["pos"]))
                    f.write("".join(f"vt {u:.5f} {1 - v:.5f}\n" for u, v in p["uv"]))
                    f.write("".join(f"vn {x:.4f} {y:.4f} {z:.4f}\n" for x, y, z in p["nrm"]))
                    f.write(f"usemtl {mat}\n")
                    t = p["tri"] + base
                    f.write("".join(f"f {a}/{a}/{a} {b}/{b}/{b} {c}/{c}/{c}\n" for a, b, c in t))
                    base += n


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
