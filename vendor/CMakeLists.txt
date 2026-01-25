add_library(vendor INTERFACE)
target_include_directories(vendor SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/tileson")

if (WIN32)
    add_library(SDL3_mixer_vendor STATIC IMPORTED)
    set_target_properties(SDL3_mixer_vendor PROPERTIES
            IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/SDL3_mixer/windows/x64/SDL3_mixer.lib"
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/SDL3_mixer/include"
    )
elseif (APPLE)
    # TODO: later add macOS dylib path
elseif (UNIX)
    # TODO: later add Linux so path
endif ()

target_link_libraries(vendor INTERFACE SDL3_mixer_vendor)