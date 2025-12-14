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
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

# Create executable
add_executable(FishingGame ${SOURCES} ${HEADERS})

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

# Copy GameEngine DLL to output directory on Windows
if(WIN32 OR MINGW)
    # Determine DLL suffix based on build type
    set(DLL_SUFFIX "")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(DLL_SUFFIX "-d")
    endif()

    # Copy GameEngine shared library to output directory using the imported target's file
    add_custom_command(TARGET FishingGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:GameEngine::GameEngine>"
            "$<TARGET_FILE_DIR:FishingGame>"
        COMMENT "Copying GameEngine runtime library to output directory"
    )

    # Copy SFML DLLs from container's SFML installation
    if(DEFINED ENV{SFML_WINDOWS_ROOT})
        foreach(component graphics window system audio)
            add_custom_command(TARGET FishingGame POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$ENV{SFML_WINDOWS_ROOT}/bin/sfml-${component}${DLL_SUFFIX}-2.dll"
                    "$<TARGET_FILE_DIR:FishingGame>"
                COMMENT "Copying SFML ${component} DLL"
            )
        endforeach()
        
        # Copy OpenAL DLL (required by SFML audio)
        add_custom_command(TARGET FishingGame POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$ENV{SFML_WINDOWS_ROOT}/bin/openal32.dll"
                "$<TARGET_FILE_DIR:FishingGame>"
            COMMENT "Copying OpenAL DLL"
        )
    endif()

    # Copy MinGW runtime DLLs
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
