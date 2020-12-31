# glhash

_glhash_ is an OpenGL compute shader implementing SHA-2-256 in GLSL.

## Introduction

_glhash_ is a OpenGL 4.x compute shader demo using _gl2_util.h_,
a nano framework for creating apps using the modern OpenGL and
GLSL shader pipeline. The `CMakeLists.txt` is intended to be used
as a template for tiny demos.

The shader implements the _SHA-2-256_ cryptographic hash function.

## Project Structure

- `src/gl4_hash.c` - OpenGL 4.x compute demo using `gl2_util.h` loader.
- `src/gl2_util.h` - header functions for OpenGL buffers and shaders.

## Project Template

This project template uses `ExternalProject`  to build and link the
_GLFW_ and _Glad_ libraries. These projects have stable APIs with stable
symbols, so in this case it is a deliberate choice to use the latest
versions. This has the advantage of avoiding submodules.

It is possible to build using the system _GLFW_ found using _PkgConfig_,
however, we believe `ExternalProject` is a good choice for OpenGL samples
that prefer the latest version of _GLFW_ versus a buggy or misconfigured
version bundled with the operating system.

The following _cmake_ configuration options are offered:

- `-DVULKAN_SDK=<path_to_vulkan_sdk>` _(used to find glslangValidator)_
- `-DSPIRV_BINARIES=OFF` _(default ON if glslangValidator is found)_
- `-DEXTERNAL_GLFW=OFF`  _(default ON, if disabled, pkg-config is used)_
- `-DEXTERNAL_GLAD=OFF` _(default ON, if disabled, glad is not used)_

## Example Invocation

To invoke the example using GLSL shader source:

> `./build/gl4_hash`

To invoke the example using SPIR-V binary module:

> `./build/gl4_hash --spir`

## Build Instructions

```
sudo apt-get install -y cmake ninja-build
cmake -G Ninja -B build .
cmake --build build
```
