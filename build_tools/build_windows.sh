#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default values
BUILD_TYPE="Release"
BUILD_SHARED="ON"
CLEAN_BUILD=false
TOOLCHAIN_FILE="cmake/toolchain-mingw64.cmake"

# By default, keep Debug/Release packages side-by-side.
# You can override with -i/--install-prefix.
INSTALL_PREFIX=""

# Help message
usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -h, --help              Show this help message"
    echo "  -t, --type TYPE         Set build type (Debug/Release) [default: Release]"
    echo "  -s, --shared            Build shared library [default: ON]"
    echo "  -c, --clean             Clean build directory"
    echo "  -i, --install-prefix    Set install prefix (default: ./package-windows)"
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

# Default build/install locations (per-config) unless the user overrides -i.
BUILD_DIR="build_windows_${BUILD_TYPE}"
if [ -z "${INSTALL_PREFIX}" ]; then
    INSTALL_PREFIX="./package_windows/${BUILD_TYPE}"
fi

# Check if CMakeLists.txt exists
if [ ! -f "CMakeLists.txt" ]; then
    echo -e "${RED}Error: CMakeLists.txt not found!${NC}"
    echo "Please run this script from the project root directory."
    exit 1
fi

# Check if toolchain file exists
if [ ! -f "$TOOLCHAIN_FILE" ]; then
    echo -e "${RED}Error: Toolchain file not found at $TOOLCHAIN_FILE${NC}"
    echo "Please ensure the MinGW toolchain file exists."
    exit 1
fi

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Cleaning Windows build directory...${NC}"
    rm -rf ${BUILD_DIR}
fi

# Create build directory if it doesn't exist
mkdir -p ${BUILD_DIR}

# Configure project with CMake for Windows
echo -e "${GREEN}Configuring project for Windows (MinGW)...${NC}"
cmake -B ${BUILD_DIR} \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DGAMEENGINE_BUILD_SHARED=$BUILD_SHARED \
    -DGAMEENGINE_BUILD_TESTS=OFF || {
        echo -e "${RED}Configuration failed!${NC}"
        exit 1
    }

# Build project
echo -e "${GREEN}Building project for Windows...${NC}"
# Use all available CPU cores for parallel compilation
NPROC=$(nproc 2>/dev/null || echo 4)
echo -e "${GREEN}Using $NPROC parallel jobs${NC}"
cmake --build ${BUILD_DIR} --config $BUILD_TYPE -j${NPROC} || {
    echo -e "${RED}Build failed!${NC}"
    exit 1
}

# Install library using CMake
echo -e "${GREEN}Installing library to ${INSTALL_PREFIX}...${NC}"
cmake --install ${BUILD_DIR} --prefix "$INSTALL_PREFIX" || {
    echo -e "${RED}Installation failed!${NC}"
    exit 1
}

echo -e "${GREEN}Windows build completed successfully!${NC}"
echo -e "${GREEN}Library installed to: ${INSTALL_PREFIX}${NC}"
echo -e "${YELLOW}Note: Make sure to include MinGW runtime DLLs if deploying:${NC}"
echo -e "${YELLOW}  - libgcc_s_seh-1.dll${NC}"
echo -e "${YELLOW}  - libstdc++-6.dll${NC}"
echo -e "${YELLOW}  - libwinpthread-1.dll${NC}"
