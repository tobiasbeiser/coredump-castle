# Coredump Castle

Coredump Castle is a student project developed for the master’s course **703333 VU Advanced C++** at the **University of
Innsbruck**.

The goal of the project is to build a game *from scratch* as a learning exercise in **modern C++ (C++23)**. To keep the
focus on core language and engine concepts, highly abstracted or performance-optimized libraries such as `entt` or
`nuklear` are intentionally **not used**.

---

## Prerequisites

* CMake
* A C++23-compatible compiler
* Git (with submodule support)
* Python 3 (for `pre-commit`)

---

## Installing Dependencies

### macOS

```bash
brew install pkg-config
brew install cmake
brew install llvm   # Required for clang-format / clang-tidy
```

Make sure the LLVM tools (`clang-format`, `clang-tidy`) are available in your `PATH`.

---

### All Platforms

#### Init Git Submodules

```bash
git submodule update --init --recursive
```

#### pre-commit

```bash
pip3 install pre-commit
pre-commit install
```

#### vcpkg

```bash
cd vcpkg
git submodule update --init
./bootstrap-vcpkg.sh   # Use .bat on Windows
```

---

## Building the Project

```bash
cmake --preset=release
cmake --build --preset=release
```

---

## Tooling & Dependencies

* **CMake** – build system
* **C++23 compiler**
* **vcpkg** – dependency management

    * `pkg-config`
* **pre-commit hooks**

    * `clang-format`
    * `clang-tidy`

---

## Notes

* The project prioritizes clarity and learning over maximum performance.
* External libraries are kept to a minimum by design.
* Code quality is enforced via automated formatting and static analysis.
