# Car Showroom Configurator

Computer Graphics Fundamentals — Final Project, topic 18.
A car rotates on a turntable in a studio; the user changes its paint colour and finish
(matte / glossy) and the lighting. C++17, OpenGL 3.3 core profile, freeglut, GLAD, GLM,
Dear ImGui.

## Build and run (Ubuntu / WSL2)

```bash
sudo apt install build-essential freeglut3-dev   # same packages as the course assignments
make            # builds ./car_showroom
make run        # or ./car_showroom — run from the project root (shaders/ and assets/ are relative paths)
```

**WSL:** if the first line printed by the program says `llvmpipe`, OpenGL runs on the CPU and is slow.
Run `export GALLIUM_DRIVER=d3d12` (or add it to `~/.bashrc`) to use the GPU.

All other libraries (GLAD, GLM, Dear ImGui, stb_image, tinyobjloader) are in `third_party/`
and build together with the project. `make clean` removes build output.

## Controls

| Input | Action |
|---|---|
| Left mouse drag / WASD / arrows | Orbit the camera |
| Mouse wheel / Q, E | Zoom |
| R | Reset camera |
| P | Next paint preset |
| M | Toggle matte / glossy |
| Space | Pause / resume turntable |
| [ / ] | Turntable speed |
| 1, 2, 3 | Toggle key / fill / rim light |
| L | Key light orbit on / off |
| V | Spin wheels |
| H | Hide / show the ImGui panel |
| Esc | Quit |

All of these are also available in the on-screen **Configurator** panel.

## Features

| Req. | Implementation | Files |
|---|---|---|
| R1 | OpenGL 3.3 core, VAO/VBO/EBO, own GLSL shaders | `Mesh.cpp`, `shaders/` |
| R2 | Studio floor, walls, turntable, floor lamps, light markers + Porsche 911 OBJ model (~198k triangles) | `Scene.cpp`, `Model.cpp` |
| R3 | Scene graph: turntable → car → body / wheel_FL…RR (each wheel spins around its own axle) | `SceneNode.cpp`, `Scene.cpp` |
| R4 | Per-fragment Blinn-Phong, 3 point lights (key light orbits, all can be toggled) | `phong.frag` |
| R5 | Mipmapped floor texture (`assets/textures/floor.png` or procedural checker) | `Texture.cpp` |
| R6 | Orbit camera; paint, finish, turntable, light controls | `App.cpp` |
| R7 | FPS in window title and in the panel | `App.cpp` |
| R8 | Makefile, same setup as the course assignments | `Makefile` |

### Planned
- [x] Car OBJ model (tinyobjloader), paint material recoloured by the configurator
- [x] Transparent pass for glass, lights and the baked contact shadow
- [ ] Skybox (cubemap)
- [ ] Stretch: cubemap reflections on the paint (Fresnel)
- [ ] Stretch: switching environment (studio / city / night)

## Project structure

```
src/        App (input, UI, frame loop), Scene (showroom content), SceneNode (scene graph),
            Model (OBJ loader), Mesh, Shader, Texture, Camera, main.cpp
shaders/    phong.vert / phong.frag
assets/     models/porsche930/ (car.obj, car.mtl, textures/), textures/, skybox/
tools/      gltf_to_obj.py — converts the Sketchfab glTF download into our OBJ
third_party/
```

### Car model pipeline

The model was downloaded from Sketchfab as glTF and converted with
`python3 tools/gltf_to_obj.py <scene.gltf> assets/models/porsche930` (needs `numpy`, `pillow`).
The script bakes node transforms, removes the 43k-triangle clear-coat shell, turns the car to face +X,
puts the tyres on y = 0, splits it into `body`, `wheel_FL/FR/RL/RR` and `shadow`, writes Blinn-Phong
material values (`Ns`, `Ks`, `d`) and keeps only baseColor textures (max 1–2K).
The converted files are committed, so building the project does not need Python.
If `assets/models/porsche930/car.obj` is missing, a placeholder car made of primitives is used.

## Team

| Member | Group | Responsibilities |
|---|---|---|
| | | |

## AI Use

| Tool | What for | What we changed |
|---|---|---|
| Claude (Anthropic) | Initial project skeleton: Makefile, Shader/Mesh/Camera/Texture/SceneNode classes, placeholder scene, ImGui panel | _fill in as you change it_ |
| Claude (Anthropic) | `tools/gltf_to_obj.py` converter, `Model` OBJ loader, transparent pass, sRGB textures | _fill in as you change it_ |
