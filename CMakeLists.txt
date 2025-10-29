
cmake_minimum_required (VERSION 4.1.0)
project ("CoredumpCastle")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

add_subdirectory(vendor)
add_subdirectory(src)
add_subdirectory(tests)


file(COPY assets DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})