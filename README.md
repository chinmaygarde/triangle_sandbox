# Triangle Sandbox

Tinkering around with [SDL3 GPU](https://wiki.libsdl.org/SDL3/CategoryGPU) and
[Shader Slang](https://shader-slang.org/).

## Prerequisites
* An Apple Silicon Mac
  * The Slang toolchain is only setup for this one target but it should be easy
    to add other platforms (by just building from source which takes a while).
    The shader reflection though is only wired up for Metal.
* Git
* Ninja
* Make
* CMake
* [optional] CCache

## Building

* `make sync` to update Git submodules.
* `make` to build and run the project with CMake & Ninja.
