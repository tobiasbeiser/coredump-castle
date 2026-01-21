# Coredump Castle

## Requirements

- CMake
- A C++23 compiler
- VCPKG
  - pkg-config
- clang-tidy

## Setup

### MacOS

```bash
brew install pkg-config
brew install cmake
```

#### vcpkg
```bash
cd vcpkg
git submodule update --init
./bootstrap-vcpkg.sh
```

## Building

```
cmake --preset=release
cmake --build --preset=release
```

