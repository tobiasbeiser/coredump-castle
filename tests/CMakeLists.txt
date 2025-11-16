file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(run_tests ${TEST_SOURCES})
target_link_libraries(run_tests PRIVATE GTest::gtest GTest::gtest_main)
target_link_libraries(run_tests PRIVATE SDL3::SDL3)

target_include_directories(run_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

include(GoogleTest)
gtest_discover_tests(run_tests)