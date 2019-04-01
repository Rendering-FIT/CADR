# CADR
CAD Rendering Vulkan library. It can be used as GPU-oriented rendering engine
for high performance complex scene rendering with GPU-based visibility culling
and LOD processing. It can very efficiently handle complicated scenes composed
of very many small objects as typical for CAD scenes.

## Compile

Prerequisities:
* Windows: Microsoft Visual C++ 2017 or
           Microsoft Visual C++ 2015 Update 3 (previous updates might work as well)
* Linux: gcc 7.3 (previous not tested)
* cmake 3.5.0 or newer
* Vulkan 1.1.70 headers or newer (previous not tested), ubuntu package name libvulkan-dev
* glm 0.9.7.0 or newer
* Linux: libx11-dev package

## Vulkan issues
If applications on Linux are failing with error message:
Failed because of Vulkan exception: vk::createInstanceUnique: ErrorIncompatibleDriver,
it might be caused by no Vulkan driver installed. On Ubuntu distribution, you may
check if there are any *.json files in /usr/share/vulkan/icd.d/.
For AMD and Intel, installation of mesa-vulkan-drivers package might solve the problem.
For Nvidia, installation of their graphics drivers might solve the problem.
