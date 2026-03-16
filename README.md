# Geometric modeling 1
This repository contains geometric modeling 1 labs. This includes lab1 and iterative design of own CAD/CAM system.
Uses its own math library (cadm).

The project uses C++ with CMake. The libraries used are Qt for windowing and widgets, OpenGL graphics API and
TBB (for std::execution library dependency; you might need to change this to compile on your system).

For details refer to the documents linked in the respective folders of the repository:
- [lab1 - ellipse raycasting](./src/app/README.md)
- [CAD/CAM system](./src/cad/README.md)

## Building and running
The project uses CMake for building. You can use any CMake-compatible build system or IDE.

Make sure to add Qt and TBB to your system and set the paths to CMake options if needed. For example set
```bash
-DCMAKE_PREFIX_PATH=C:\Qt\6.10.2\msvc2022_64
```
to add Qt to the CMake search path on Windows.

The project was tested to compile and run with CLion on Windows 11 and Linux (Arch and Ubuntu 22.04 LTS).
