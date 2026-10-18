# Vulkan declarations

`vulkan_core.h`, `vk_platform.h` and `LICENSE.txt` are unmodified files from
[Khronos Vulkan-Headers v1.2.198](https://github.com/KhronosGroup/Vulkan-Headers/tree/v1.2.198).
They are Apache-2.0 licensed. The backend uses Vulkan 1.0 core calls and loads
the driver's system loader dynamically. No SDK or import library is needed.

The glslang compiler is a development tool only, not linked or distributed in
the game. Embedded SPIR-V is generated from this project's existing HLSL by
`scripts/compile_vulkan_shaders.py`. See `VULKAN_RENDERER.md` for provenance.
