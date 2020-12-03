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

## Build Instructions

```
sudo apt-get install -y cmake ninja-build
cmake -G Ninja -B build .
cmake --build build
```
