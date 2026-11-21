# Car Showroom Configurator

Computer Graphics Fundamentals — Final Project, topic 18.

A car rotates on a turntable in a showroom. The user picks one of four cars, changes its paint
colour and finish (matte / glossy), switches the environment (studio, day, night) and the lights.
C++17, OpenGL 3.3 core profile, freeglut, GLAD, GLM, Dear ImGui.

## Build and run (Ubuntu / WSL2)

```bash
sudo apt install build-essential freeglut3-dev   # same packages as the course assignments
make            # builds ./car_showroom
./car_showroom  # run from the project root (shaders/ and assets/ are relative paths)
```

`make run` builds and runs, `make clean` removes build output. All other libraries
(GLAD, GLM, Dear ImGui, stb_image, tinyobjloader) are in `third_party/` and build with the project.
Start-up takes a few seconds: four car models are loaded.

**WSL:** if the first line printed by the program says `llvmpipe`, OpenGL runs on the CPU and is slow.
Run `export GALLIUM_DRIVER=d3d12` (or add it to `~/.bashrc`) so that the GPU is used.

## Controls

| Input | Action |
|---|---|
| Left mouse drag / WASD / arrows | Orbit the camera |
| Mouse wheel / Q, E | Zoom |
| R | Reset camera |
| C | Next car |
| P | Next paint preset |
| M | Toggle matte / glossy |
| N | Next environment (studio, day, night) |
| Space | Pause / resume turntable |
| [ / ] | Turntable speed |
| 1, 2, 3 | Toggle key / fill / rim light |
| 0 | All point lights on / off |
| 4 | Ceiling spotlights on / off |
| K | Headlights on / off |
| L | Key light orbit on / off |
| V | Spin wheels |
| F | Anti-aliasing (MSAA) on / off |
| H | Hide / show the ImGui panel |
| Esc | Quit |

Everything is also available in the on-screen **Configurator** panel (car list, paint colour picker,
shininess, specular and reflection sliders, environment buttons, light switches, turntable speed).

## Features

| Req. | Implementation | Files |
|---|---|---|
| R1 | OpenGL 3.3 core, VAO/VBO/EBO, own GLSL shaders, no fixed-function calls | `Mesh.cpp`, `Shader.cpp`, `shaders/` |
| R2 | Showroom hall (walls, ceiling, panoramic window wall, columns, pendant lamps, plants, table, counter, sign, car posters, rope barrier around the turntable), turntable, 4 OBJ car models | `Scene.cpp`, `Model.cpp` |
| R3 | Scene graph with MVP from GLM: turntable → car → body / wheel_FL…RR; wheels spin around their own axle | `SceneNode.cpp`, `Scene.cpp` |
| R4 | Per-fragment Blinn-Phong: 3 point lights (key light orbits), 3 ceiling spotlights, 2 headlights; every light can be switched off | `phong.frag`, `Scene.cpp` |
| R5 | Mipmapped textures: floor, car textures (sRGB), showroom sign, car posters; cubemap skyboxes | `Texture.cpp`, `Cubemap.cpp` |
| R6 | Orbit camera (mouse + keyboard, frame-rate independent); car, paint, finish, environment, lights, turntable controls | `Camera.cpp`, `App.cpp` |
| R7 | FPS and frame time in the window title and in the panel; MSAA can be switched off | `App.cpp` |
| R8 | Makefile, same setup as the course assignments | `Makefile` |

### Stretch goals

- **Reflections from a cubemap** — paint, glass and the floor reflect the environment; Fresnel term
  makes the edges more reflective; reflection strength per material and a slider in the panel.
- **Switching environment** — *Studio* (closed dark hall), *Day* (bright hall, city behind the
  panoramic windows), *Night* (night city with lit windows, dim interior). Sky, lights, wall, ceiling
  and floor colours change together.
- **Headlights as spotlights** — two spot lights attached to the car follow the turntable rotation;
  on automatically at night, key K otherwise.

## Project structure

```
src/        App (input, UI, frame loop), Scene (showroom content), SceneNode (scene graph),
            Model (OBJ loader), Mesh, Shader, Texture, Cubemap, Skybox, Camera,
            Environment.h (studio / day / night presets), main.cpp
shaders/    phong.vert / phong.frag (lighting, reflections), skybox.vert / skybox.frag
assets/     models/ (porsche930, supra93, r34, aventador), skybox/ (city, night), textures/ (sign, posters/)
tools/      gltf_to_obj.py, make_city_skybox.py, make_sign.py
third_party/
```

### Asset pipeline

- **Cars:** downloaded from Sketchfab as glTF and converted with
  `python3 tools/gltf_to_obj.py <car id> <scene.gltf>` (needs `numpy`, `pillow`). Per car the script
  removes hidden parts (interior, engine, clear-coat shell), turns the car to face +X, puts the tyres on
  y = 0, merges the body paint into one `paint` material, splits the wheels into `wheel_FL/FR/RL/RR`
  and keeps baseColor textures (max 1–2K). The OBJ loader re-centres each wheel on its bounding box,
  so it rotates around its own axle.
- **Skyboxes:** `python3 tools/make_city_skybox.py` draws the day and night city panoramas
  (6 cubemap faces each). The studio sky is a procedural gradient built in `Cubemap::gradient`.
- **Sign:** `python3 tools/make_sign.py` renders the sign texture.
- **Posters:** screenshots of the four cars taken in this application, with a caption bar.

The generated files are committed, so building the project does not need Python.

## Performance

| Machine | GPU | Car | FPS |
|---|---|---|---|
| Laptop (Amina Dossan), WSL2 | Intel Iris Xe (D3D12) | Porsche 911 | 80–90 |

## Team

| Member | Group | Responsibilities |
|---|---|---|
| Amina Dossan | SE-2422 | Core (build, window, Shader, Mesh, Camera), car models, car selection and posters, performance, repository |
| Artyom Karmykov | SE-2422 | Scene graph, textures, skybox, showroom hall, window wall, decor, barrier |
| Yerasyl Ginayat | SE-2422 | Model loader, environments (studio / day / night), city skyline, spotlights, headlights |
| Nazerke Abzhamal | SE-2422 | Blinn-Phong shaders, App and ImGui panel, reflections and Fresnel, interior per environment, sign |

## AI Use

Claude (Anthropic) was used throughout the project to write project's base code. Every step was reviewed, built,
run and tested by the team member who committed it.

| Tool | What for | What the team did |
|---|---|---|
| Claude (Anthropic) | Project skeleton: Makefile, Shader, Mesh, Camera, Texture, SceneNode, placeholder scene, ImGui panel | Chose the topic and scope, split the work, set up the repository and `.gitignore` / `.gitattributes`, tested the build on WSL and fixed the GPU issue (`GALLIUM_DRIVER=d3d12`) |
| Claude (Anthropic) | `gltf_to_obj.py` converter, OBJ loader, multi-car support | Searched Sketchfab, compared candidate models, rejected low-quality ones, checked licences and polygon counts, chose the four cars, tested every car in the app |
| Claude (Anthropic) | Frame time display, MSAA toggle, uniform cache | Measured FPS on the team's laptops |
