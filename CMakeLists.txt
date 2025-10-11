cmake_minimum_required(VERSION 3.21)

project(CodeRedGenerator VERSION 1.1.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")  # `/MD`
set(CMAKE_GENERATOR_TOOLSET "v143" CACHE STRING "Toolset version") # 142 = RL's version

set(ARCH_NAME "x64") # Hardcode architecture
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${ARCH_NAME}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${ARCH_NAME}/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${ARCH_NAME}/$<CONFIG>")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_link_options(/DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF)

add_compile_options(${compile_options})
add_compile_definitions(UNICODE _UNICODE)

if (CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR
    "This project requires a 64-bit build.\n"
    "Make sure your toolchain is configured to use a 64-bit compiler."
  )
endif()

add_library(${PROJECT_NAME}
    SHARED
    src/pch.hpp
    src/pch.cpp
    src/dllmain.cpp
    src/Engine/Engine.cpp
    src/Engine/Pattern.cpp
    src/Engine/RocketLeague/Configuration.cpp
    src/Engine/RocketLeague/GameDefines.cpp
    src/Engine/RocketLeague/PiecesOfCode.cpp
    src/Engine/RocketLeague/Globals.cpp
    src/Framework/Member.cpp
    src/Framework/Printer.cpp
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/src/pch.hpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/src
)
