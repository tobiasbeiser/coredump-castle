add_subdirectory(SDL EXCLUDE_FROM_ALL)


set(SDLIMAGE_VENDORED ON)
set(SDLIMAGE_AVIF OFF)
set(SDLIMAGE_TIF OFF)
set(SDLIMAGE_WEBP OFF)
add_subdirectory(SDL_image EXCLUDE_FROM_ALL)

include(FetchContent)

FetchContent_Declare(
	glm
	GIT_REPOSITORY	"https://github.com/g-truc/glm.git"
	GIT_TAG 	0af55ccecd98d4e5a8d1fad7de25ba429d60e863 #refs/tags/1.0.1
)
set(GLM_BUILD_TESTS OFF)
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(glm)


add_library(vendor INTERFACE)
target_link_libraries(vendor INTERFACE SDL3::SDL3 SDL3_image::SDL3_image glm)
target_include_directories(vendor SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/tileson")