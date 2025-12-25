#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Change to the script directory (Example_Game)
cd "$SCRIPT_DIR"

# Default values
BUILD_TYPE="Debug"
BUILD_DIR="build"
CLEAN_BUILD=false
TOOLCHAIN_FILE="../cmake/toolchain-mingw64.cmake"

# Help message
usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -h, --help              Show this help message"
    echo "  -t, --type TYPE         Set build type (Debug/Release) [default: Debug]"
    echo "  -c, --clean             Clean build directory"
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
        -c|--clean)
            CLEAN_BUILD=true
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            usage
            exit 1
            ;;
    esac
    shift
done

# Normalize build type (capitalize first letter)
BUILD_TYPE="$(tr '[:lower:]' '[:upper:]' <<< ${BUILD_TYPE:0:1})$(tr '[:upper:]' '[:lower:]' <<< ${BUILD_TYPE:1})"

# Validate build type
if [[ "$BUILD_TYPE" != "Debug" && "$BUILD_TYPE" != "Release" ]]; then
    echo -e "${RED}Error: Invalid build type '$BUILD_TYPE'. Must be Debug or Release.${NC}"
    exit 1
fi

# Set GameEngine directory based on build type
PARENT_PACKAGE_DIR="../package_windows/${BUILD_TYPE}"

# Check if CMakeLists.txt exists
if [ ! -f "CMakeLists.txt" ]; then
    echo -e "${RED}Error: CMakeLists.txt not found!${NC}"
    echo "Please run this script from the Example_Game directory."
    exit 1
fi

# Backwards-compatible fallback to the legacy flat folder.
if [ ! -d "$PARENT_PACKAGE_DIR" ]; then
    LEGACY_PARENT_PACKAGE_DIR="../package_windows"
    if [ -d "$LEGACY_PARENT_PACKAGE_DIR" ]; then
        PARENT_PACKAGE_DIR="$LEGACY_PARENT_PACKAGE_DIR"
    else
        echo -e "${RED}Error: GameEngine library not found at $PARENT_PACKAGE_DIR${NC}"
        echo ""
        echo "Please build locally first:"
        echo "  cd .. && ./build_tools/build.sh --windows --type $BUILD_TYPE"
        exit 1
    fi
fi

echo -e "${GREEN}Using local build from $PARENT_PACKAGE_DIR${NC}"
GAMEENGINE_DIR="$PARENT_PACKAGE_DIR"

# Help CMake find packaged dependencies when cross-compiling.
SPDLOG_DIR="${GAMEENGINE_DIR}/lib/cmake/spdlog"

# Check if toolchain file exists
if [ ! -f "$TOOLCHAIN_FILE" ]; then
    echo -e "${RED}Error: Toolchain file not found at $TOOLCHAIN_FILE${NC}"
    echo "Please ensure the MinGW toolchain file exists."
    exit 1
fi

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf ${BUILD_DIR}
fi

# Create build directory if it doesn't exist
mkdir -p ${BUILD_DIR}

# Configure project with CMake for Windows cross-compilation
echo -e "${GREEN}Configuring Example Game for Windows (MinGW)...${NC}"
cmake_args=(
    -B "${BUILD_DIR}"
    -G Ninja
    "-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}"
    "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
    "-DCMAKE_PREFIX_PATH=${GAMEENGINE_DIR};${GAMEENGINE_DIR}/lib/cmake"
    "-DGameEngine_DIR=${GAMEENGINE_DIR}/lib/cmake/GameEngine"
    "-Dbox2d_DIR=${GAMEENGINE_DIR}/lib/cmake/box2d"
    "-Dspdlog_DIR=${SPDLOG_DIR}"
)

cmake "${cmake_args[@]}" || {
        exit 1
    }

# Build project
echo -e "${GREEN}Building Example Game for Windows...${NC}"
cmake --build ${BUILD_DIR} --config $BUILD_TYPE -j8 || {
    echo -e "${RED}Build failed!${NC}"
    exit 1
}

# Copy GameEngine runtime DLLs next to the executable.
# (When cross-compiling, CMake cannot reliably compute runtime DLL deps.)
echo -e "${GREEN}Copying GameEngine runtime DLLs...${NC}"
if [ -d "${GAMEENGINE_DIR}/bin" ]; then
    cp -f "${GAMEENGINE_DIR}/bin/"*.dll "${BUILD_DIR}/" 2>/dev/null || true
fi

# Copy Example assets next to the executable so relative paths like
#   assets/textures/boat.png
# work even when running from the build folder.
echo -e "${GREEN}Copying Example assets...${NC}"
if [ -d "${SCRIPT_DIR}/assets" ]; then
    rm -rf "${BUILD_DIR}/assets" 2>/dev/null || true
    cp -r "${SCRIPT_DIR}/assets" "${BUILD_DIR}/assets"
fi

# Flatten build directory: move everything from bin to build root and clean up
echo -e "${GREEN}Flattening build directory...${NC}"
if [ -d "${BUILD_DIR}/bin" ]; then
    # Copy everything from bin to build root
    cp -r ${BUILD_DIR}/bin/* ${BUILD_DIR}/
    # Remove bin directory
    rm -rf ${BUILD_DIR}/bin
fi

# Remove lib directory
rm -rf ${BUILD_DIR}/lib

# Remove CMake files and directories
rm -rf ${BUILD_DIR}/CMakeFiles
rm -f ${BUILD_DIR}/cmake_install.cmake
rm -f ${BUILD_DIR}/CMakeCache.txt
rm -f ${BUILD_DIR}/Makefile
rm -f ${BUILD_DIR}/build.ninja
rm -f ${BUILD_DIR}/rules.ninja
rm -f ${BUILD_DIR}/.ninja_deps
rm -f ${BUILD_DIR}/.ninja_log

echo -e "${GREEN}Build completed successfully!${NC}"
echo -e "${GREEN}Executable location: ${BUILD_DIR}/FishingGame.exe${NC}"
echo -e "${GREEN}All required DLLs have been copied to the build directory${NC}"
