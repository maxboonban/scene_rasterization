# Scene Rasterization of HDR-NeRF Indoor Scene
Real-time OpenGL rasterizer built with Qt that renders an indoor HDR-NeRF dining room scene from Blender-exported geometry and textures.

---

## Overview

- Renders a pre-made HDR-NeRF indoor scene (dining room) using **rasterization**, not NeRF.
- Scene exported from Blender as `.obj` + `.mtl` with:
  - Positions, normals, UVs
  - Textures for materials
- Uses `tinyobjloader` to load triangle meshes into an OpenGL rasterizer.
- Built as a Qt `QOpenGLWidget` app with support for:
  - Analytic primitives (sphere, cube, cone, cylinder)
  - FBO-based post-processing (bloom, blur)
  - Camera Movements

---

## Repo Structure (high-level)

```text
scene_rasterization/
├── CMakeLists.txt
├── glm/                    # GLM math library
├── glew/                   # GLEW source & headers
├── resources/
│   └── shaders/            # GLSL shaders
└── src/
    ├── main.cpp
    ├── mainwindow.*
    ├── realtime.*          # Core renderer (QOpenGLWidget)
    ├── realtimefbo.*       # FBO & post-processing
    ├── realtimephong.*     # Phong shading helpers
    ├── realtimeshadowmap.* # Shadow mapping
    ├── realtimetrimeshes.* # Triangle mesh rendering
    ├── realtimecameramovement.*
    ├── camera/
    ├── shapes/
    └── utils/
        ├── scenedata.*, scenefilereader.*, sceneparser.*
        ├── shaderloader.*
        ├── aspectratiowidget.hpp
        └── tiny_obj_loader.h
