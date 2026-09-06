cmake_minimum_required(VERSION 3.21)

project(RLSDKGenerator VERSION 1.1.5)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")  # `/MD`
set(CMAKE_GENERATOR_TOOLSET "v143" CACHE STRING "Toolset version") # 142 = RL's version

set(ARCH_NAME "x64") # Hardcode architecture
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${ARCH_NAME}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${ARCH_NAME}/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${ARCH_NAME}/$<CONFIG>")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# suppress Clang-specific warnings (that MSVC ignores)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -Wno-nontrivial-memcall
        -Wno-sizeof-array-argument
        -Wno-absolute-value
        -Wno-deprecated-enum-enum-conversion
        -Wno-invalid-offsetof
        -Wno-extra-qualification
    )
endif()

add_link_options(/DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF)

add_compile_options(${compile_options})
add_compile_definitions(UNICODE _UNICODE)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
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

if(NOT "$ENV{CI}" STREQUAL "true")
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
        set(_post_build_script "${CMAKE_SOURCE_DIR}/scripts/post-build-linux.sh")
        if(EXISTS "${_post_build_script}")
            add_custom_command(
                TARGET ${PROJECT_NAME}
                POST_BUILD
                COMMAND bash "${_post_build_script}" "$<TARGET_FILE:${PROJECT_NAME}>"
                COMMENT "Running local (linux) post-build hook after building ${PROJECT_NAME}"
            )
        endif()
    endif()
endif()
