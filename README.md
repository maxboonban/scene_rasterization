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

Dependencies
	•	Qt 6 (e.g. Qt 6.9.2):
	•	Core, Gui, OpenGL, OpenGLWidgets, Xml
	•	CMake ≥ 3.16
	•	OpenGL (system framework on macOS)
	•	GLM (vendored in glm/)
	•	GLEW (vendored in glew/)
	•	tinyobjloader (header-only, vendored in utils/)

All external libraries are included in the repo; you only need Qt + CMake.

Adding the Scene Assets (IMPORTANT)

The main demo uses a dining room scene from the HDR-NeRF dataset, exported from Blender as OBJ/MTL.

These files are not stored in the repo and must be added manually:
	•	diningroom.obj
	•	diningroom.mtl

Where to put them
	1.	First, configure the project so the build directory exists, e.g.:
<repo-root>/build/build-projects_realtime-Qt_6_9_2_for_macOS-Debug
	2.	Copy both files into that build directory:
  .../build-projects_realtime-Qt_6_9_2_for_macOS-Debug/
    diningroom.obj
    diningroom.mtl
    projects_realtime          # executable
    CMakeCache.txt
    ...
	3.	Ensure:
	•	Filenames are exactly diningroom.obj and diningroom.mtl.
	•	The .mtl name referenced inside diningroom.obj matches the actual .mtl filename.

The renderer expects these files in the build dir when loading with tinyobjloader.
