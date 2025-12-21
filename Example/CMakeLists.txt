cmake_minimum_required(VERSION 3.28)

project(FishingGameExample
    VERSION 1.0.0
    DESCRIPTION "Example game using GameEngine library"
    LANGUAGES CXX)

# Define DEBUG macro for Debug builds only (works with multi-config generators)
add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Enable parallel compilation for MSVC
if(MSVC)
    add_compile_options(/MP)
endif()

# Note: CMAKE_PREFIX_PATH and GameEngine_DIR are set by the build script

# Find GameEngine package
find_package(GameEngine REQUIRED)

# Glob source files
# Use CONFIGURE_DEPENDS so CMake regenerates when files are added/removed.
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "src/*.h")

# Create executable
add_executable(FishingGame ${SOURCES} ${HEADERS})

# Allow simple includes like "Boat.h" even when files are organized into src/* subfolders.
# This avoids forcing `#include "boat/Boat.h"` style includes.
set(EXAMPLE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src")
file(GLOB _EXAMPLE_SRC_CHILDREN CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*")
foreach(_child ${_EXAMPLE_SRC_CHILDREN})
    if(IS_DIRECTORY "${_child}")
        list(APPEND EXAMPLE_INCLUDE_DIRS "${_child}")
    endif()
endforeach()
target_include_directories(FishingGame PRIVATE ${EXAMPLE_INCLUDE_DIRS})

# Link against GameEngine and its dependencies
target_link_libraries(FishingGame
    PRIVATE
    GameEngine::GameEngine
)

# Set target properties
set_target_properties(FishingGame PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)

# Copy runtime files + assets on Windows
if(WIN32 OR MINGW)
    # Native Windows builds: let CMake compute runtime DLL dependencies.
    if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
        add_custom_command(TARGET FishingGame POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:FishingGame>
                $<TARGET_FILE_DIR:FishingGame>
            COMMAND_EXPAND_LISTS
            COMMENT "Copying runtime DLLs to output directory"
        )
    endif()

    # Cross-compile builds (e.g. Linux->Windows in container): keep explicit copying.
    # SFML 3 official shared binaries typically use a "-3" suffix; copy only if present.
    if(CMAKE_CROSSCOMPILING AND DEFINED ENV{SFML_WINDOWS_ROOT})
        set(DLL_SUFFIX "")
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(DLL_SUFFIX "-d")
        endif()

        foreach(component graphics window system audio)
            set(_sfml_dll "$ENV{SFML_WINDOWS_ROOT}/bin/sfml-${component}${DLL_SUFFIX}-3.dll")
            if(EXISTS "${_sfml_dll}")
                add_custom_command(TARGET FishingGame POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_sfml_dll}"
                        "$<TARGET_FILE_DIR:FishingGame>"
                    COMMENT "Copying SFML ${component} DLL"
                )
            endif()
        endforeach()

        set(_openal_dll "$ENV{SFML_WINDOWS_ROOT}/bin/openal32.dll")
        if(EXISTS "${_openal_dll}")
            add_custom_command(TARGET FishingGame POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_openal_dll}"
                    "$<TARGET_FILE_DIR:FishingGame>"
                COMMENT "Copying OpenAL DLL"
            )
        endif()
    endif()

    # Copy MinGW runtime DLLs (used in the cross-compile container)
    add_custom_command(TARGET FishingGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "/usr/lib/gcc/x86_64-w64-mingw32/13-posix/libgcc_s_seh-1.dll"
            "$<TARGET_FILE_DIR:FishingGame>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "/usr/lib/gcc/x86_64-w64-mingw32/13-posix/libstdc++-6.dll"
            "$<TARGET_FILE_DIR:FishingGame>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "/usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll"
            "$<TARGET_FILE_DIR:FishingGame>"
        COMMENT "Copying MinGW runtime DLLs"
    )

    # Copy assets folder to output directory
    add_custom_command(TARGET FishingGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:FishingGame>/assets"
        COMMENT "Copying assets to output directory"
    )
endif()

# Print configuration info
message(STATUS "Example Game Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  GameEngine Dir: ${GameEngine_DIR}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
