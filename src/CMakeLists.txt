
file(GLOB_RECURSE ENGINE_SRC "Engine/*.cpp" "Engine/*.hpp")
file(GLOB_RECURSE GAME_SRC "Game/*.cpp" "Game/*.hpp")

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE main.cpp ${ENGINE_SRC}  ${GAME_SRC} )
#target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Engine ${CMAKE_CURRENT_SOURCE_DIR}/Game)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}  # Project root
)


target_link_libraries(${PROJECT_NAME}  PRIVATE $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 glm::glm)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3_ttf::SDL3_ttf)

target_link_libraries(${PROJECT_NAME} PRIVATE vendor)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXCEPTIONS ON)