
file(GLOB_RECURSE ENGINE_SRC "Engine/*.cpp" "Engine/*.hpp")
file(GLOB_RECURSE GAME_SRC "Game/*.cpp" "Game/*.hpp")

add_library(GameLib STATIC ${ENGINE_SRC} ${GAME_SRC})

target_include_directories(GameLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(GameLib PUBLIC 
    SDL3::SDL3 
    $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>
    glm::glm
    nlohmann_json::nlohmann_json
    imgui::imgui
    SDL3_ttf::SDL3_ttf
    TBB::tbb
    vendor
)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE GameLib)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXCEPTIONS ON)