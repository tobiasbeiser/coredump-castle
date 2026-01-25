add_library(vendor INTERFACE)
target_include_directories(vendor SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/tileson")


# Force consistent static, non-framework builds on macOS
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CMAKE_MACOSX_RPATH ON)

# SDL projects default to frameworks on macOS — disable explicitly
set(SDL3MIXER_BUILD_FRAMEWORK OFF CACHE BOOL "" FORCE)
set(SDL3MIXER_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Disable X11/Wayland features for headless CI build
set(SDL_X11 OFF CACHE BOOL "" FORCE)
set(SDL_X11_XCURSOR OFF CACHE BOOL "" FORCE)
set(SDL_X11_XRANDR OFF CACHE BOOL "" FORCE)
set(SDL_X11_XI OFF CACHE BOOL "" FORCE)
set(SDL_X11_XINERAMA OFF CACHE BOOL "" FORCE)
set(SDL_WAYLAND OFF CACHE BOOL "" FORCE)
set(SDL_IBUS OFF CACHE BOOL "" FORCE)

add_subdirectory(SDL3_mixer)