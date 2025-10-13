
cmake_minimum_required (VERSION 4.1.0)
project ("SDLTestGame")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(vendor)
add_subdirectory(src)

if(MSVC)
    target_compile_options(vendor INTERFACE /external:W0)
endif()

file(COPY assets DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})


# Exclude out/ from analysis by marking source files as SYSTEM or SKIP_ANALYSIS
#file(GLOB_RECURSE OUT_DIR_SOURCES "${CMAKE_SOURCE_DIR}/out/*")
#file(GLOB_RECURSE OUT_DIR_SOURCES "${CMAKE_SOURCE_DIR}/vendor/*")

#foreach(file_path IN LISTS OUT_DIR_SOURCES)
#    set_source_files_properties(${file_path} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
#    set_source_files_properties(${file_path} PROPERTIES SKIP_ANALYSIS ON)
#endforeach()
#if(MSVC)
#    add_compile_options(/wd26440)  # Suppress C26440 globally
#endif()