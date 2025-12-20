enable_testing()

# Set up Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Common Directories
set(TEST_SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../include")
set(COMPONENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../components")
set(SYSTEM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../systems")
set(UTILITY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../utility")
set(PHYSICS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../physics")

# Copy test data to build directory (optional)
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_data")
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_data DESTINATION ${CMAKE_BINARY_DIR}/bin/Debug)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_data DESTINATION ${CMAKE_BINARY_DIR}/bin/Release)
endif()

# Add test executable
file(GLOB_RECURSE TEST_FILES "${TEST_SOURCES_DIR}/*.cpp")

add_executable(unit_tests ${TEST_FILES})

# Add library directories
target_link_directories(unit_tests PRIVATE
    "${CMAKE_BINARY_DIR}/lib/Debug"
)

# Link with the main library and Google Test
target_link_libraries(unit_tests PRIVATE
    GameEngine
    GTest::gtest
    GTest::gtest_main
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    ImGui-SFML
)

# When engine coverage is enabled, unit_tests must link the coverage runtime
# and (optionally) be instrumented so header/inlined engine logic executed from
# tests is attributed correctly to engine headers.
if(GAMEENGINE_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(GAMEENGINE_COVERAGE_INSTRUMENT_TESTS)
        target_compile_options(unit_tests PRIVATE --coverage)
    endif()

    target_link_options(unit_tests PRIVATE --coverage)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_libraries(unit_tests PRIVATE gcov)
    endif()
endif()

# Link Windows-specific libraries only when building for Windows
if(WIN32 OR MINGW)
    target_link_libraries(unit_tests PRIVATE
        opengl32
        winmm
        gdi32
    )
endif()

# Include directories for tests
target_include_directories(unit_tests PRIVATE
    ${INCLUDE_DIR}
    ${COMPONENT_DIR}
    ${SYSTEM_DIR}
    ${UTILITY_DIR}
    ${PHYSICS_DIR}
    $<TARGET_PROPERTY:spdlog::spdlog,INTERFACE_INCLUDE_DIRECTORIES>
)

# Copy required DLLs to the test executable directory on Windows.
# Prefer target-based dependency discovery for native Windows builds.
if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
    add_custom_command(TARGET unit_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:unit_tests>
            $<TARGET_FILE_DIR:unit_tests>
        COMMAND_EXPAND_LISTS
        COMMENT "Copying runtime DLLs to test executable directory"
    )
endif()