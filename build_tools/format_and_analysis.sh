#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if clang-format and cppcheck are installed
if ! command -v clang-format &> /dev/null; then
    echo -e "${RED}clang-format could not be found! Please install clang-format.${NC}"
    exit 1
fi

if ! command -v cppcheck &> /dev/null; then
    echo -e "${RED}cppcheck could not be found!${NC}"
    exit 1
fi


# Function to format files
format_files() {
    echo -e "${YELLOW}Formatting files...${NC}"

    # Use the same find filter as check_formatting so untracked files are also formatted
    files_to_format=$(find . -type f \( -name "*.cpp" -o -name "*.h" \) \
        -not -path "./build/*" \
        -not -path "./build_linux/*" \
        -not -path "./build_windows/*" \
        -not -path "./deps_cache*/*" \
        -not -path "./package*/*" \
        -not -path "./coverage_report/*" \
        -not -path "./tests/*")

    if [ -z "$files_to_format" ]; then
        echo -e "${YELLOW}No files to format${NC}"
        return
    fi

    # Format each file that needs it (skip empty list)
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

    # Use find command like CI/CD does
    find . -type f \( -name "*.cpp" -o -name "*.h" \) \
        -not -path "./build/*" \
        -not -path "./build_linux/*" \
        -not -path "./build_windows/*" \
        -not -path "./deps_cache*/*" \
        -not -path "./package*/*" \
        -not -path "./coverage_report/*" \
        -not -path "./tests/*" \
        -exec clang-format -style=file --dry-run --Werror {} +

    local result=$?
    if [ $result -eq 0 ]; then
        echo -e "${GREEN}Formatting check passed!${NC}"
    else
        echo -e "${RED}Formatting check failed! Files need to be formatted.${NC}"
        echo -e "${YELLOW}Run './build_tools/format_and_analysis.sh' to auto-format files.${NC}"
        exit 1
    fi
}

static_analysis() {
    echo -e "${YELLOW}Running static analysis...${NC}"

    # Create report header
    cat > static_analysis_report.md << EOF
# Static Analysis Report

**Date:** $(date)
**Tool:** cppcheck
**Configuration:** --enable=all --check-level=exhaustive

---

## Results

EOF

    # Get list of tracked source and header files
    tracked_files=$(git ls-files '*.cpp' '*.h')

    # Ignore all of the files in the tests/ directory
    tracked_files=$(echo "$tracked_files" | grep -v '^tests/')

    if [ -z "$tracked_files" ]; then
        echo -e "${YELLOW}No files to analyze${NC}"
        echo -e "\n---\n\n**Status:** ⚠️ NO FILES\n" >> static_analysis_report.md
        return
    fi

    # Run cppcheck on tracked files only
    echo "$tracked_files" | xargs cppcheck --enable=all --error-exitcode=1 --check-level=exhaustive --language=c++ --std=c++17 --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=unusedStructMember --suppress=noExplicitConstructor --suppress=unmatchedSuppression --suppress=missingInclude --inline-suppr -I include -I include/components -I include/systems 2>&1 | tee -a static_analysis_report.md

    local result=${PIPESTATUS[0]}

    # Add summary to report
    if [ $result -eq 0 ]; then
        echo -e "\n---\n\n**Status:** ✅ PASSED\n" >> static_analysis_report.md
        echo -e "${GREEN}Static analysis complete! Report saved to static_analysis_report.md${NC}"
    else
        echo -e "\n---\n\n**Status:** ❌ FAILED\n" >> static_analysis_report.md
        echo -e "${RED}Static analysis failed! Report saved to static_analysis_report.md${NC}"
        exit 1
    fi
}

format_files
check_formatting
static_analysis