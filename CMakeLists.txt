cmake_minimum_required(VERSION 3.25)
project(risc_v_test)

set(CMAKE_CXX_STANDARD 23)

set(IMGUI_SFML_FIND_SFML OFF)
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/thirdparty/imgui")
set(SPDLOG_FMT_EXTERNAL ON)

add_subdirectory(thirdparty/googletest)
add_subdirectory(thirdparty/fmt)
add_subdirectory(thirdparty/imgui-sfml)
add_subdirectory(thirdparty/SFML)
add_subdirectory(thirdparty/spdlog)
add_subdirectory(thirdparty/stuff/libs/core)
add_subdirectory(thirdparty/stuff/libs/random)
add_subdirectory(thirdparty/stuff/libs/thread)

add_executable(${PROJECT_NAME} main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE include thirdparty/imgui_club)
target_link_libraries(${PROJECT_NAME}
        SFML::System SFML::Window SFML::Graphics
        ImGui-SFML::ImGui-SFML
        fmt::fmt spdlog::spdlog

        stuff_core stuff_random stuff_thread
        )

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_options(${PROJECT_NAME} PUBLIC -fsanitize=address -fsanitize=undefined)
    target_link_options(${PROJECT_NAME} PUBLIC -fsanitize=address -fsanitize=undefined)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_options(${PROJECT_NAME} PUBLIC -ffast-math -march=native -mtune=native -fopenmp)
    target_link_options(${PROJECT_NAME} PUBLIC -fopenmp)
endif()

add_executable(${PROJECT_NAME}_tests
        tests/rvc.cpp
        tests/rvi.cpp
        tests/rvm.cpp
        )

target_compile_options(${PROJECT_NAME}_tests PRIVATE -fsanitize=address -fsanitize=undefined)
target_link_options(${PROJECT_NAME}_tests PRIVATE -fsanitize=address -fsanitize=undefined)
target_include_directories(${PROJECT_NAME}_tests PRIVATE include)

target_link_libraries(${PROJECT_NAME}_tests
        fmt::fmt
        gtest gtest_main
        stuff_core stuff_random
        )
