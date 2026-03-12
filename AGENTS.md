# AGENTS.md

## Project Snapshot

- Language: C99
- Build system: CMake
- Windowing: SDL2
- Vulkan loader helpers: bundled `volk`
- Main targets: `sample_minimal`, `sample_dyn_render`
- Shader pipeline: GLSL in `shaders/`, compiled to SPIR-V during CMake build
- Minimum runtime API: Vulkan 1.3

## Fast Start

1. Inspect `CMakeLists.txt` for build flags, dependencies, and shader compilation rules.
2. Read `common.h` and `common.c` first. Most project behavior lives there.
3. Compare `sample_minimal.c` and `sample_dyn_render.c` to understand the two rendering paths.
4. If rendering output changes, also inspect `shaders/base.vert` and `shaders/base.frag`.

## Build And Run

Debug:

```bash
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/sample_minimal
./build/sample_dyn_render
```

Release:

```bash
cmake -G Ninja -B build_release -DCMAKE_BUILD_TYPE=Release
cmake --build build_release
```

Sanitizers:

```bash
cmake -G Ninja -B build_asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITAIZE=ON
cmake --build build_asan
```

Important: run executables from their build directory or keep the working directory such that `shaders/base.vert.spv` and `shaders/base.frag.spv` resolve correctly.

## Architecture Notes

- `common.c` handles:
  - SDL initialization
  - Vulkan instance creation
  - extension/layer probing
  - physical device selection
  - logical device creation
  - swapchain creation and recreation
  - command buffers, fences, semaphores
  - per-frame submit/present flow
- `sample_minimal.c` adds:
  - `VkRenderPass`
  - framebuffer-backed render pass recording
- `sample_dyn_render.c` adds:
  - dynamic rendering via `vkCmdBeginRendering`
  - Synchronization2 image barriers
- The scene is procedural:
  - no vertex buffers
  - geometry indices and positions are embedded in the vertex shader
  - animation data is passed through push constants

## Operational Expectations

- Prefer small, explicit changes. This codebase values direct Vulkan setup over abstractions.
- Preserve the difference between the two samples. Shared logic belongs in `common.c`; rendering-path specifics belong in the sample files.
- If changing synchronization, present modes, swapchain recreation, or queue selection, review both samples and the shared frame loop.
- If changing shader file names or locations, update the runtime load paths and CMake shader build rules together.
- Existing build directories may already contain artifacts; avoid assuming they are fresh.

## Useful Verification

- Rebuild after code changes:

```bash
cmake --build build
```

- If CMake logic changed, reconfigure explicitly:

```bash
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug
```

- For rendering-path edits, validate both targets build:

```bash
cmake --build build --target sample_minimal sample_dyn_render
```

## Known Constraints

- `rg` may be unavailable in the environment; use `find`, `grep`, or `sed` as fallback.
- Network access may be restricted, so prefer local inspection over fetching external docs.
- There are no automated tests beyond CI compilation. Verification is mostly successful build plus local runtime checks on a Vulkan-capable machine.
