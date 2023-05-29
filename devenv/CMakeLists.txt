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

project(risc_v_test_prog C CXX ASM)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/link.ld)

add_compile_options(-march=rv64imafdc -mabi=lp64d)
add_compile_options(-ffunction-sections -fdata-sections)
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

file(GLOB_RECURSE SOURCES "${CMAKE_SORUCE_DIR}/src/*.*")

add_executable(${PROJECT_NAME}.elf
        src/startup.s
        src/entry.cpp
        src/special.cpp
        src/prog.cpp
        ${LINKER_SCRIPT})
set_target_properties(${PROJECT_NAME}.elf PROPERTIES LINKER_LANGUAGE CXX)

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE}
Building ${BIN_FILE}")
