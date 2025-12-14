#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default values
BUILD_TYPE="Debug"
BUILD_SHARED="OFF"
BUILD_DIR="build_linux"
RUN_TESTS=true
CLEAN_BUILD=false
INSTALL_PREFIX="./package_linux"
RUN_COVERAGE=false

generate_coverage_report() {
    # Coverage is best-effort and only runs when tests are enabled.
    if [ "$RUN_TESTS" != true ]; then
        return 0
    fi

    if ! command -v lcov &> /dev/null || ! command -v genhtml &> /dev/null; then
        echo -e "${YELLOW}Skipping coverage: lcov/genhtml not installed${NC}"
        return 0
    fi

    echo -e "${GREEN}Generating coverage report...${NC}"

    local COVERAGE_BUILD_DIR="build_linux_coverage"
    local COVERAGE_OUTPUT_DIR="coverage_report"

    rm -rf "${COVERAGE_BUILD_DIR}" "${COVERAGE_OUTPUT_DIR}" coverage.info coverage_filtered.info
    mkdir -p "${COVERAGE_OUTPUT_DIR}"

    echo -e "${GREEN}Configuring coverage build...${NC}"
    cmake -B "${COVERAGE_BUILD_DIR}" \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DGAMEENGINE_BUILD_SHARED=$BUILD_SHARED \
        -DCMAKE_C_FLAGS="--coverage" \
        -DCMAKE_CXX_FLAGS="--coverage" \
        -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
        -DCMAKE_SHARED_LINKER_FLAGS="--coverage" \
        -DDEPS_CACHE_DIR="${PWD}/deps_cache_linux_coverage" \
        || { echo -e "${YELLOW}Coverage configuration failed; skipping coverage.${NC}"; return 0; }

    echo -e "${GREEN}Building coverage targets...${NC}"
    local NPROC
    NPROC=$(nproc 2>/dev/null || echo 4)
    cmake --build "${COVERAGE_BUILD_DIR}" --config $BUILD_TYPE -j"${NPROC}" \
        || { echo -e "${YELLOW}Coverage build failed; skipping coverage.${NC}"; return 0; }

    # Determine test executable name based on platform/build layout
    local TEST_EXEC=""
    if [ -f "./${COVERAGE_BUILD_DIR}/bin/unit_tests.exe" ]; then
        TEST_EXEC="./${COVERAGE_BUILD_DIR}/bin/unit_tests.exe"
    elif [ -f "./${COVERAGE_BUILD_DIR}/bin/${BUILD_TYPE}/unit_tests.exe" ]; then
        TEST_EXEC="./${COVERAGE_BUILD_DIR}/bin/${BUILD_TYPE}/unit_tests.exe"
    elif [ -f "./${COVERAGE_BUILD_DIR}/bin/unit_tests" ]; then
        TEST_EXEC="./${COVERAGE_BUILD_DIR}/bin/unit_tests"
    elif [ -f "./${COVERAGE_BUILD_DIR}/bin/${BUILD_TYPE}/unit_tests" ]; then
        TEST_EXEC="./${COVERAGE_BUILD_DIR}/bin/${BUILD_TYPE}/unit_tests"
    else
        echo -e "${YELLOW}Coverage test executable not found; skipping coverage.${NC}"
        return 0
    fi

    echo -e "${GREEN}Running tests (coverage build)...${NC}"
    "${TEST_EXEC}" --gtest_output="xml:${COVERAGE_BUILD_DIR}/test_results.xml" \
        || { echo -e "${YELLOW}Coverage test run failed; skipping coverage.${NC}"; return 0; }

    echo -e "${GREEN}Capturing coverage data...${NC}"
    # Some gtest macro expansions can confuse line-end detection; ignore mismatch errors.
    lcov --capture \
        --directory "${COVERAGE_BUILD_DIR}" \
        --output-file coverage.info \
        --ignore-errors inconsistent,negative,mismatch \
        --rc geninfo_unexecuted_blocks=1 \
        || { echo -e "${YELLOW}Coverage capture failed; skipping coverage.${NC}"; return 0; }

    echo -e "${GREEN}Filtering coverage to engine sources only...${NC}"
    lcov --remove coverage.info \
        "/usr/*" \
        "*/deps_cache_*/*" \
        "*/_deps/*" \
        "*/tests/*" \
        "*/Example/*" \
        --output-file coverage_filtered.info \
        --ignore-errors unused \
        || { echo -e "${YELLOW}Coverage filtering failed; skipping coverage.${NC}"; return 0; }

    echo -e "${GREEN}Generating HTML report...${NC}"
    genhtml coverage_filtered.info \
        --output-directory "${COVERAGE_OUTPUT_DIR}" \
        --legend \
        --title "GameEngine Coverage" \
        || { echo -e "${YELLOW}HTML coverage report generation failed; skipping coverage.${NC}"; return 0; }

    echo -e "${GREEN}Coverage report generated at ./${COVERAGE_OUTPUT_DIR}/index.html${NC}"
    return 0
}

# Help message
usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -h, --help              Show this help message"
    echo "  -t, --type TYPE         Set build type (Debug/Release)"
    echo "  -s, --shared            Build shared library"
    echo "  -n, --no-tests          Skip building and running tests"
    echo "  -g, --coverage          Generate coverage report (runs tests in coverage build)"
    echo "  -c, --clean             Clean build directory"
    echo "  -i, --install-prefix    Set install prefix (default: ./package-linux)"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            exit 0
            ;;
        -t|--type)
            BUILD_TYPE="$2"
            shift
            ;;
        -s|--shared)
            BUILD_SHARED="ON"
            ;;
        -n|--no-tests)
            RUN_TESTS=false
            ;;
        -g|--coverage)
            RUN_COVERAGE=true
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            ;;
        -i|--install-prefix)
            INSTALL_PREFIX="$2"
            shift
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            usage
            exit 1
            ;;
    esac
    shift
done

# If coverage is requested, run tests in the coverage build (once), not the normal build.
NORMAL_BUILD_RUN_TESTS=$RUN_TESTS
if [ "$RUN_COVERAGE" = true ]; then
    NORMAL_BUILD_RUN_TESTS=false
fi

# Check if CMakeLists.txt exists
if [ ! -f "CMakeLists.txt" ]; then
    echo -e "${RED}Error: CMakeLists.txt not found!${NC}"
    echo "Please run this script from the project root directory."
    exit 1
fi

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf ${BUILD_DIR}
fi

# Create build directory if it doesn't exist
mkdir -p ${BUILD_DIR}

# Configure project with CMake
echo -e "${GREEN}Configuring project...${NC}"
cmake -B ${BUILD_DIR} -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DGAMEENGINE_BUILD_SHARED=$BUILD_SHARED || { echo -e "${RED}Configuration failed!${NC}"; exit 1; }

# Build project
echo -e "${GREEN}Building project...${NC}"
# Use all available CPU cores for parallel compilation
NPROC=$(nproc 2>/dev/null || echo 4)
echo -e "${GREEN}Using $NPROC parallel jobs${NC}"
cmake --build ${BUILD_DIR} --config $BUILD_TYPE -j${NPROC} || { echo -e "${RED}Build failed!${NC}"; exit 1; }

# Run tests if enabled
if [ "$NORMAL_BUILD_RUN_TESTS" = true ]; then
    echo -e "${GREEN}Running tests...${NC}"
    # Create Testing directory structure
    mkdir -p ${BUILD_DIR}/Testing/Temporary

    # Determine test executable name based on platform
    if [ -f "./${BUILD_DIR}/bin/unit_tests.exe" ]; then
        TEST_EXEC="./${BUILD_DIR}/bin/unit_tests.exe"
    elif [ -f "./${BUILD_DIR}/bin/${BUILD_TYPE}/unit_tests.exe" ]; then
        TEST_EXEC="./${BUILD_DIR}/bin/${BUILD_TYPE}/unit_tests.exe"
    elif [ -f "./${BUILD_DIR}/bin/unit_tests" ]; then
        TEST_EXEC="./${BUILD_DIR}/bin/unit_tests"
    elif [ -f "./${BUILD_DIR}/bin/${BUILD_TYPE}/unit_tests" ]; then
        TEST_EXEC="./${BUILD_DIR}/bin/${BUILD_TYPE}/unit_tests"
    else
        echo -e "${RED}Test executable not found!${NC}"
        exit 1
    fi

    # Run tests and capture output
    "$TEST_EXEC" --gtest_output="xml:${BUILD_DIR}/test_results.xml" 2>&1 | tee ${BUILD_DIR}/Testing/Temporary/LastTest.log || { echo -e "${RED}Tests failed!${NC}"; exit 1; }
fi

# Generate coverage report (Linux-only, best-effort)
if [ "$RUN_COVERAGE" = true ]; then
    generate_coverage_report
fi

# Install library using CMake
echo -e "${GREEN}Installing library to ${INSTALL_PREFIX}...${NC}"
cmake --install ${BUILD_DIR} --prefix "$INSTALL_PREFIX" || { echo -e "${RED}Installation failed!${NC}"; exit 1; }

echo -e "${GREEN}Build completed successfully!${NC}"
echo -e "${GREEN}Library installed to: ${INSTALL_PREFIX}${NC}"