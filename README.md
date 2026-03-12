# Vulkan Beginner

Small Vulkan 1.3 learning project in C with SDL2 for windowing and `volk` for function loading.

The repository contains two executable samples that share the same platform and device setup code:

- `sample_minimal`: classic render-pass based pipeline.
- `sample_dyn_render`: the same scene rendered with Vulkan dynamic rendering and `vkQueueSubmit2` / Synchronization2.

Both samples open an SDL2 Vulkan window, create a swapchain, compile GLSL shaders to SPIR-V during the build, and draw a rotating lit pyramid generated directly in the vertex shader.

## What The Project Demonstrates

- Vulkan 1.3 instance and device initialization
- SDL2 window + Vulkan surface integration
- Physical device selection and queue-family discovery
- Swapchain creation and recreation for fullscreen toggle
- Graphics pipeline creation with push constants
- Per-frame synchronization with fences and semaphores
- Shader compilation with `glslc`
- Two rendering paths: render pass and dynamic rendering

## Repository Layout

- `sample_minimal.c`: render-pass based sample entry point
- `sample_dyn_render.c`: dynamic rendering sample entry point
- `common.c`, `common.h`: shared Vulkan/SDL2 bootstrap, swapchain, synchronization, frame loop
- `shader_io.c`, `shader_io.h`: SPIR-V loading helpers
- `shaders/base.vert`, `shaders/base.frag`: GLSL shaders
- `volk/`: bundled `volk` sources
- `.github/workflows/ci.yaml`: Linux CI build

## Requirements

- CMake 3.31+
- A C99-capable compiler
- Ninja or another CMake generator
- Vulkan SDK with:
  - Vulkan 1.3 headers/loader
  - `glslc`
  - validation layers for debug work
- SDL2 development package

The code requires Vulkan 1.3 at runtime. CI currently builds on Ubuntu with `clang`, `libc++`, `vulkan-sdk`, and `libsdl2-dev`.

## Build

Debug build with Ninja:

```bash
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Release build:

```bash
cmake -G Ninja -B build_release -DCMAKE_BUILD_TYPE=Release
cmake --build build_release
```

AddressSanitizer can be enabled with:

```bash
cmake -G Ninja -B build_asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITAIZE=ON
cmake --build build_asan
```

## Run

Run from the build directory so the executables can find `shaders/*.spv` using relative paths:

```bash
./build/sample_minimal
./build/sample_dyn_render
```

Controls:

- `Esc`: quit
- `F`: toggle fullscreen and recreate the swapchain

In debug builds, `VALIDATION_LAYERS` is enabled by CMake and the app tries to enable Khronos validation plus extra validation features when available.

## Notes On Implementation

- The project targets clarity over abstraction. Most Vulkan setup is intentionally explicit.
- `common.c` owns the shared lifecycle: instance, device, swapchain, command buffers, frame submission, presentation, and cleanup.
- `sample_minimal.c` creates a traditional `VkRenderPass` and framebuffers.
- `sample_dyn_render.c` skips render-pass objects during recording and uses `vkCmdBeginRendering` with image layout transitions via Synchronization2.
- The shaders use push constants for time and aspect ratio, so there are no descriptor sets yet.

## Current Limitations

- Linux CI is present; runtime behavior is not covered by automated tests
- No geometry buffers, descriptor sets, textures, depth buffer, or camera controls yet
- The samples are intended for local experimentation, not as a reusable engine layer

