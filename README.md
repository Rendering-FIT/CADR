# CADR
CAD Renderer Vulkan library. It can be used as GPU-oriented rendering engine
for high performance complex scene rendering with GPU-based visibility culling
and LOD processing. It can very efficiently handle complicated scenes composed
of very many small objects as typical for CAD scenes.

## Compile

Prerequisities:
* Windows: Microsoft Visual C++ 2022
* Linux: gcc
* cmake
* development libraries (included in 3rdParty folder):
** Vulkan development files (ubuntu package name libvulkan-dev, Vulkan-SDK on Windows)
** glm 0.9.7.0 or newer
** boost
** glslang-tools
* to compile examples on Linux (depends of choosen GUI_TYPE):
** X11: libx11-dev package
** Wayland:
*** libwayland-dev
*** wayland-protocols
*** libdecor-0-0 (on systems where server-side decorations are not supported)
** SDL3:
*** libsdl3?
** SDL2:
*** libsdl2-2.0-0
