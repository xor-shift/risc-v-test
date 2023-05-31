set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)

cmake_minimum_required(VERSION 3.25)

set(CMAKE_C_COMPILER riscv64-unknown-elf-gcc)
set(CMAKE_CXX_COMPILER riscv64-unknown-elf-g++)
set(CMAKE_ASM_COMPILER riscv64-unknown-elf-gcc)
set(CMAKE_AR riscv64-unknown-elf-ar)
set(CMAKE_OBJCOPY riscv64-unknown-elf-objcopy)
set(CMAKE_OBJDUMP riscv64-unknown-elf-objdump)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(SIZE riscv64-unknown-elf-size)

project(rvtest C CXX ASM)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/link.ld)

add_compile_options(-march=rv64imafdc -mabi=lp64d)
add_compile_options(--save-temps -ffunction-sections -fdata-sections -mno-relax)
add_link_options(-ffunction-sections -fdata-sections -Wl,-gc-sections,--print-memory-usage)
add_link_options(-T ${LINKER_SCRIPT})

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "release build")
    add_compile_options(-O3)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "release build with debug info")
    add_compile_options(-O3 -g)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "release build with minimum size")
    add_compile_options(-Os)
else()
    message(STATUS "debug build")
    add_compile_options(-O0)
endif()

add_executable(${PROJECT_NAME}.elf src/startup.s src/entry.cpp src/main_gol.cpp)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMAND readelf -SW $<TARGET_FILE:${PROJECT_NAME}.elf>
    )
