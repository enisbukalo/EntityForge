cmake_minimum_required(VERSION 3.28)

# Set version information
set(GAMEENGINE_VERSION_MAJOR 0)
set(GAMEENGINE_VERSION_MINOR 1)
set(GAMEENGINE_VERSION_PATCH 0)
set(GAMEENGINE_VERSION ${GAMEENGINE_VERSION_MAJOR}.${GAMEENGINE_VERSION_MINOR}.${GAMEENGINE_VERSION_PATCH})

project(GameEngine
    VERSION ${GAMEENGINE_VERSION}
    DESCRIPTION "A 2D Game Engine with Entity Component System"
    LANGUAGES CXX)

# Build options
option(GAMEENGINE_BUILD_SHARED "Build GameEngine as shared library" OFF)
option(GAMEENGINE_BUILD_TESTS "Build test programs" ON)
option(GAMEENGINE_INSTALL "Generate installation target" ON)
option(GAMEENGINE_ENABLE_COVERAGE "Enable coverage instrumentation for engine targets" OFF)
option(GAMEENGINE_COVERAGE_INSTRUMENT_TESTS "Instrument unit tests during coverage builds (improves header/inlined engine coverage)" ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find threading library
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# Enable parallel compilation for MSVC
if(MSVC)
    add_compile_options(/MP)
endif()

# Cache directory for dependencies - separate for different toolchains
if(MINGW OR WIN32)
    set(DEPS_CACHE_DIR "${CMAKE_SOURCE_DIR}/deps_cache_windows" CACHE PATH "Directory for cached dependencies")
else()
    set(DEPS_CACHE_DIR "${CMAKE_SOURCE_DIR}/deps_cache_linux" CACHE PATH "Directory for cached dependencies")
endif()

include(FetchContent)

# Configure dependency caching
set(FETCHCONTENT_BASE_DIR ${DEPS_CACHE_DIR})

# Find SFML
# - Linux builds typically use shared libs (system/packaged).
# - Our Windows cross-compile Docker image builds SFML as static, so request static there.
if(MINGW)
    set(SFML_STATIC_LIBRARIES TRUE)
else()
    set(SFML_STATIC_LIBRARIES FALSE)
endif()
find_package(SFML 3.0.2 REQUIRED COMPONENTS Graphics Window System Audio)

# Find OpenGL (required for ImGui-SFML)
find_package(OpenGL REQUIRED)

# Fetch spdlog for logging (header-only mode to avoid symbol conflicts)
set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(SPDLOG
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
    GIT_SHALLOW ON)

FetchContent_MakeAvailable(SPDLOG)

# Fetch Box2D (v3.1 for physics engine)
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(box2d
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG v3.1.1
    GIT_SHALLOW ON)

FetchContent_MakeAvailable(box2d)

# Fetch uuid_v4 (header-only UUID v4 generator)
FetchContent_Declare(uuid_v4
    GIT_REPOSITORY https://github.com/crashoz/uuid_v4.git
    GIT_TAG master
    GIT_SHALLOW ON)

FetchContent_MakeAvailable(uuid_v4)

# Fetch nlohmann/json (header-only JSON library)
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW ON)

FetchContent_MakeAvailable(nlohmann_json)

# Fetch ImGui (newer version required by ImGui-SFML v3.0)
FetchContent_Declare(IMGUI
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9b
    GIT_SHALLOW ON)

FetchContent_MakeAvailable(IMGUI)

# Fetch ImGui-SFML (v3.0 for SFML 3.x compatibility)
FetchContent_Declare(IMGUI_SFML
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG v3.0
    GIT_SHALLOW ON)

FetchContent_GetProperties(IMGUI_SFML)
if(NOT imgui_sfml_POPULATED)
    FetchContent_Populate(IMGUI_SFML)

    # Manually build ImGui-SFML without using its CMakeLists.txt find_package
    add_library(ImGui-SFML STATIC
        ${imgui_sfml_SOURCE_DIR}/imgui-SFML.cpp
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    )

    target_include_directories(ImGui-SFML PUBLIC
        $<BUILD_INTERFACE:${imgui_sfml_SOURCE_DIR}>
        $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/ImGui-SFML>
    )

    target_link_libraries(ImGui-SFML PUBLIC
        SFML::Graphics
        SFML::Window
        SFML::System
        OpenGL::GL
    )

    target_compile_features(ImGui-SFML PUBLIC cxx_std_17)

    # Create an alias to match the original target name
    add_library(ImGui-SFML::ImGui-SFML ALIAS ImGui-SFML)
endif()

# Common Directories
set(ENGINE_SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(ENGINE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(ENGINE_INPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/Input")
set(ENGINE_COMPONENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/components")
set(ENGINE_OBJECTIVES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/objectives")
set(ENGINE_SYSTEM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/systems")
set(ENGINE_UI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/ui")
set(ENGINE_UTILITY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/utility")
set(ENGINE_PHYSICS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/physics")

# Create Game Engine library
file(GLOB_RECURSE ENGINE_SOURCES "${ENGINE_SOURCES_DIR}/*.cpp")
file(GLOB_RECURSE ENGINE_HEADERS "${ENGINE_INCLUDE_DIR}/*.h")

# Components are header-only/data-only; avoid compiling any legacy/accidental .cpp files.
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "src[\\\\/]components[\\\\/].*\\.cpp$")

set(ENGINE_HEADERS ${ENGINE_HEADERS})

# Create library target
if(GAMEENGINE_BUILD_SHARED)
    add_library(GameEngine SHARED ${ENGINE_SOURCES} ${ENGINE_HEADERS})
    set_target_properties(GameEngine PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    add_library(GameEngine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})
endif()

add_library(GameEngine::GameEngine ALIAS GameEngine)

# Set library properties
set_target_properties(GameEngine PROPERTIES
    VERSION ${GAMEENGINE_VERSION}
    SOVERSION ${GAMEENGINE_VERSION_MAJOR}
    DEBUG_POSTFIX "-d"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib/Debug"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib/Release"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/lib/Debug"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/lib/Release"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON)

target_include_directories(GameEngine
    PUBLIC
    $<BUILD_INTERFACE:${ENGINE_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${ENGINE_INPUT_DIR}>
    $<BUILD_INTERFACE:${ENGINE_COMPONENT_DIR}>
    $<BUILD_INTERFACE:${ENGINE_OBJECTIVES_DIR}>
    $<BUILD_INTERFACE:${ENGINE_SYSTEM_DIR}>
    $<BUILD_INTERFACE:${ENGINE_SYSTEM_DIR}/serialization>
    $<BUILD_INTERFACE:${ENGINE_UI_DIR}>
    $<BUILD_INTERFACE:${ENGINE_UTILITY_DIR}>
    $<BUILD_INTERFACE:${ENGINE_PHYSICS_DIR}>
    $<BUILD_INTERFACE:${uuid_v4_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/GameEngine/include>
    $<INSTALL_INTERFACE:include/GameEngine/include/Input>
    $<INSTALL_INTERFACE:include/GameEngine/components>
    $<INSTALL_INTERFACE:include/GameEngine/objectives>
    $<INSTALL_INTERFACE:include/GameEngine/systems>
    $<INSTALL_INTERFACE:include/GameEngine/systems/serialization>
    $<INSTALL_INTERFACE:include/GameEngine/ui>
    $<INSTALL_INTERFACE:include/GameEngine/utility>
    $<INSTALL_INTERFACE:include/GameEngine/physics>
    $<INSTALL_INTERFACE:include>
)

# Coverage (apply only to engine targets; tests remain non-instrumented)
if(GAMEENGINE_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(GameEngine PRIVATE --coverage)
    # Link options matter for shared builds (static libs don't link), but harmless otherwise.
    target_link_options(GameEngine PRIVATE --coverage)
endif()

# nlohmann/json is header-only; include it directly to avoid export/install dependency issues.
target_include_directories(GameEngine PRIVATE $<BUILD_INTERFACE:${nlohmann_json_SOURCE_DIR}/include>)

# Allow setting AVX2 (or the platform equivalent) only for the Guid translation unit which
# uses the header-only uuid_v4 implementation with AVX intrinsics.
include(CheckCXXCompilerFlag)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_MAVX2)
    if (COMPILER_SUPPORTS_MAVX2)
        set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/utility/Guid.cpp PROPERTIES COMPILE_FLAGS "-mavx2")
    endif()
elseif (MSVC)
    check_cxx_compiler_flag("/arch:AVX2" COMPILER_SUPPORTS_ARCH_AVX2)
    if (COMPILER_SUPPORTS_ARCH_AVX2)
        set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/utility/Guid.cpp PROPERTIES COMPILE_FLAGS "/arch:AVX2")
    endif()
endif()

# Link against dependencies
if(GAMEENGINE_BUILD_SHARED AND MINGW AND SFML_STATIC_LIBRARIES)
    # IMPORTANT (MinGW cross-compile): SFML is built as static libs in the Docker image.
    # If we export SFML in the GameEngine link interface while GameEngine itself is a DLL,
    # consumers (like Example) will ALSO link SFML static libs into the EXE.
    # That duplicates SFML global state across DLL + EXE and can cause hard crashes.
    target_link_libraries(GameEngine
        PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        SFML::Audio
        ImGui-SFML
        Threads::Threads
        box2d
    )

    # But GameEngine public headers include SFML headers, so consumers still need SFML include dirs.
    # Provide them as include directories without adding SFML static libs to the consumer link line.
    target_include_directories(GameEngine PUBLIC
        $<TARGET_PROPERTY:SFML::Graphics,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:SFML::Window,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:SFML::System,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:SFML::Audio,INTERFACE_INCLUDE_DIRECTORIES>
    )
else()
    target_link_libraries(GameEngine
        PUBLIC
        SFML::Graphics
        SFML::Window
        SFML::System
        SFML::Audio
        ImGui-SFML
        Threads::Threads
        box2d
    )
endif()

target_link_libraries(GameEngine PRIVATE spdlog::spdlog)

# Link Windows-specific libraries only when building for Windows
if(WIN32 OR MINGW)
    target_link_libraries(GameEngine
        PRIVATE
        opengl32
        winmm
        gdi32
        dbghelp  # Required for StackTrace symbol resolution
    )
endif()

# Installation configuration
if(GAMEENGINE_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    # Dependencies required by consumers of the installed GameEngine package.
    # Keep this aligned with the SFML version/components used by the build.
    set(GAMEENGINE_SFML_MIN_VERSION 3.0.2)
    set(GAMEENGINE_SFML_COMPONENTS Graphics Window System Audio)

    set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/GameEngine)

    # Install library and dependencies
    install(TARGETS GameEngine ImGui-SFML
        EXPORT GameEngineTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/GameEngine
    )

    # Install headers
    # 1) Install top-level public headers + Input/* into include/GameEngine/include
    #    (exclude subfolders that are installed separately to keep the package tidy).
    install(DIRECTORY
        ${ENGINE_INCLUDE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/GameEngine/include
        FILES_MATCHING PATTERN "*.h"
        PATTERN "components" EXCLUDE
        PATTERN "systems" EXCLUDE
        PATTERN "ui" EXCLUDE
        PATTERN "utility" EXCLUDE
        PATTERN "physics" EXCLUDE
        PATTERN "objectives" EXCLUDE
    )

    # 2) Install structured subfolders so consumers can include headers without a prefix
    #    when we add those subfolders to INTERFACE_INCLUDE_DIRECTORIES.
    install(DIRECTORY ${ENGINE_COMPONENT_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/GameEngine/components
        FILES_MATCHING PATTERN "*.h"
    )
    install(DIRECTORY ${ENGINE_OBJECTIVES_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/GameEngine/objectives
        FILES_MATCHING PATTERN "*.h"
    )
    install(DIRECTORY ${ENGINE_SYSTEM_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/GameEngine/systems
        FILES_MATCHING PATTERN "*.h"
    )
    install(DIRECTORY ${ENGINE_UI_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/GameEngine/ui
        FILES_MATCHING PATTERN "*.h"
    )
    install(DIRECTORY ${ENGINE_UTILITY_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/GameEngine/utility
        FILES_MATCHING PATTERN "*.h"
    )
    install(DIRECTORY ${ENGINE_PHYSICS_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/GameEngine/physics
        FILES_MATCHING PATTERN "*.h"
    )

    # Install ImGui and ImGui-SFML headers
    install(DIRECTORY
        ${imgui_SOURCE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ImGui-SFML
        FILES_MATCHING PATTERN "*.h"
    )
    install(FILES
        ${imgui_sfml_SOURCE_DIR}/imgui-SFML.h
        ${imgui_sfml_SOURCE_DIR}/imgui-SFML_export.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ImGui-SFML
    )

    # Export targets
    install(EXPORT GameEngineTargets
        FILE GameEngineTargets.cmake
        NAMESPACE GameEngine::
        DESTINATION ${INSTALL_CONFIGDIR}
    )

    # Create version file
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/GameEngineConfigVersion.cmake"
        VERSION ${GAMEENGINE_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    # Configure the package config file (used by consumers via find_package(GameEngine)).
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GameEngineConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/GameEngineConfig.cmake"
        INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
    )

    # Install config files
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/GameEngineConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/GameEngineConfigVersion.cmake"
        DESTINATION ${INSTALL_CONFIGDIR}
    )
endif()

# Add tests if enabled
if(GAMEENGINE_BUILD_TESTS)
    # Configure GTest - build as static libraries to avoid DLL issues
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(INSTALL_GMOCK OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    # Temporarily disable shared library building for Google Test
    set(BUILD_SHARED_LIBS_BACKUP ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF)

    add_subdirectory(tests)

    # Restore BUILD_SHARED_LIBS setting
    set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_BACKUP})
endif()
