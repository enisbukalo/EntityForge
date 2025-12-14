#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
BUILD_LINUX=false
BUILD_WINDOWS=false
BUILD_TYPE="Debug"
CLEAN_BUILD=false
RUN_TESTS=true
RUN_COVERAGE=false
SCRIPT_DIR="$(dirname "$0")"

# Help message
usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -h, --help              Show this help message"
    echo "  -l, --linux             Build for Linux"
    echo "  -w, --windows           Build for Windows (cross-compile with MinGW)"
    echo "  -a, --all               Build for all platforms (Linux + Windows)"
    echo "  -t, --type TYPE         Set build type (Debug/Release) [default: Debug]"
    echo "  -c, --clean             Clean build directories"
    echo "  -n, --no-tests          Skip building and running tests (Linux only)"
    echo "  -g, --coverage          Generate coverage report (Linux only; runs tests in coverage build)"
    echo ""
    echo "Examples:"
    echo "  $0 --linux              Build for Linux only"
    echo "  $0 --linux --coverage   Build for Linux and generate coverage report"
    echo "  $0 --windows            Build for Windows only"
    echo "  $0 --all                Build for both platforms"
    echo "  $0 -l -t Release        Build Linux Release version"
    echo "  $0 -a -c                Clean and build all platforms"
}

# Parse command line arguments
if [ $# -eq 0 ]; then
    echo -e "${RED}Error: No build target specified${NC}"
    usage
    exit 1
fi

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            exit 0
            ;;
        -l|--linux)
            BUILD_LINUX=true
            ;;
        -w|--windows)
            BUILD_WINDOWS=true
            ;;
        -a|--all)
            BUILD_LINUX=true
            BUILD_WINDOWS=true
            ;;
        -t|--type)
            BUILD_TYPE="$2"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            ;;
        -n|--no-tests)
            RUN_TESTS=false
            ;;
        -g|--coverage)
            RUN_COVERAGE=true
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            usage
            exit 1
            ;;
    esac
    shift
done

# Check if build scripts exist
if [ ! -f "${SCRIPT_DIR}/build_linux.sh" ]; then
    echo -e "${RED}Error: Linux build script not found at ${SCRIPT_DIR}/build_linux.sh${NC}"
    exit 1
fi

if [ ! -f "${SCRIPT_DIR}/build_windows.sh" ]; then
    echo -e "${RED}Error: Windows build script not found at ${SCRIPT_DIR}/build_windows.sh${NC}"
    exit 1
fi

# Make sure build scripts are executable
chmod +x "${SCRIPT_DIR}/build_linux.sh"
chmod +x "${SCRIPT_DIR}/build_windows.sh"

# Build counter
BUILD_SUCCESS=0
BUILD_FAILED=0

# Build for Linux
if [ "$BUILD_LINUX" = true ]; then
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}Building for Linux...${NC}"
    echo -e "${BLUE}========================================${NC}"

    LINUX_ARGS="-t $BUILD_TYPE"
    if [ "$CLEAN_BUILD" = true ]; then
        LINUX_ARGS="$LINUX_ARGS -c"
    fi
    if [ "$RUN_TESTS" = false ]; then
        LINUX_ARGS="$LINUX_ARGS -n"
    fi
    if [ "$RUN_COVERAGE" = true ]; then
        LINUX_ARGS="$LINUX_ARGS -g"
    fi

    if bash "${SCRIPT_DIR}/build_linux.sh" $LINUX_ARGS; then
        echo -e "${GREEN}Linux build completed successfully!${NC}"
        ((BUILD_SUCCESS++))
    else
        echo -e "${RED}Linux build failed!${NC}"
        ((BUILD_FAILED++))
    fi
    echo ""
fi

# Build for Windows
if [ "$BUILD_WINDOWS" = true ]; then
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}Building for Windows...${NC}"
    echo -e "${BLUE}========================================${NC}"

    WINDOWS_ARGS="-t $BUILD_TYPE"
    if [ "$CLEAN_BUILD" = true ]; then
        WINDOWS_ARGS="$WINDOWS_ARGS -c"
    fi

    if bash "${SCRIPT_DIR}/build_windows.sh" $WINDOWS_ARGS; then
        echo -e "${GREEN}Windows build completed successfully!${NC}"
        ((BUILD_SUCCESS++))
    else
        echo -e "${RED}Windows build failed!${NC}"
        ((BUILD_FAILED++))
    fi
    echo ""
fi

# Summary
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Build Summary${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}Successful builds: $BUILD_SUCCESS${NC}"
if [ $BUILD_FAILED -gt 0 ]; then
    echo -e "${RED}Failed builds: $BUILD_FAILED${NC}"
    exit 1
else
    echo -e "${GREEN}All builds completed successfully!${NC}"
    exit 0
fi
