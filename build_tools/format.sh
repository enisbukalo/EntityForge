#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo -e "${RED}clang-format could not be found! Please install clang-format.${NC}"
    exit 1
fi

# Function to format files
format_files() {
    echo -e "${YELLOW}Formatting files...${NC}"

    # Only format engine + Example sources
    files_to_format=$(find src include Example -type f \( -name "*.cpp" -o -name "*.h" \) \
        -not -path "Example/build/*")

    if [ -z "$files_to_format" ]; then
        echo -e "${YELLOW}No files to format${NC}"
        return
    fi

    for file in $files_to_format; do
        echo "Formatting $file..."
        clang-format -style=file -i "$file"
    done

    echo -e "${GREEN}Formatting complete!${NC}"
}

# Function to check formatting (matches CI/CD behavior)
check_formatting() {
    echo -e "${YELLOW}Checking code formatting (clang-format)...${NC}"

    # Only check engine + Example sources
    find src include Example -type f \( -name "*.cpp" -o -name "*.h" \) \
        -not -path "Example/build/*" \
        -exec clang-format -style=file --dry-run --Werror {} +

    local result=$?
    if [ $result -eq 0 ]; then
        echo -e "${GREEN}Formatting check passed!${NC}"
    else
        echo -e "${RED}Formatting check failed! Files need to be formatted.${NC}"
        echo -e "${YELLOW}Run './build_tools/format.sh' to auto-format files.${NC}"
        exit 1
    fi
}

format_files
check_formatting
