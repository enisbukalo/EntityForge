FROM ubuntu:24.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install essential build tools and dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    build-essential \
    cmake \
    git \
    clang \
    clang-format \
    lcov \
    libjson-xs-perl \
    ninja-build \
    pkg-config \
    wget \
    unzip \
    libx11-dev \
    libxrandr-dev \
    libxcursor-dev \
    libxi-dev \
    libudev-dev \
    libgl1-mesa-dev \
    libopenal-dev \
    libvorbis-dev \
    libflac-dev \
    libfreetype6-dev \
    gcc-mingw-w64-x86-64 \
    g++-mingw-w64-x86-64 \
    libssl-dev \
    dos2unix \
    cppcheck \
    && update-ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Install SFML 3.0.2 for Linux (so find_package(SFML 3.0.2 ...) succeeds)
RUN wget -O /tmp/sfml.tar.gz https://www.sfml-dev.org/files/SFML-3.0.2-linux-gcc-64-bit.tar.gz && \
    tar -xzf /tmp/sfml.tar.gz -C /tmp && \
    cp -r /tmp/SFML-3.0.2/include /usr/local/ && \
    cp -r /tmp/SFML-3.0.2/lib /usr/local/ && \
    rm -rf /tmp/SFML-3.0.2 /tmp/sfml.tar.gz && \
    ldconfig

# Set up working directory
WORKDIR /app

# Build and install SFML 3.0.2 for Windows (MinGW GCC 13) into a prefix that CMake can discover.
# SFML 3 prebuilt Windows packages are for GCC 14.2 UCRT; our cross-compiler in this image is GCC 13,
# so we build SFML from source to avoid toolchain/ABI mismatches.
RUN mkdir -p /opt/sfml-windows && \
    wget -O /tmp/sfml-src.zip https://www.sfml-dev.org/files/SFML-3.0.2-sources.zip && \
    unzip /tmp/sfml-src.zip -d /tmp && \
    cmake -S /tmp/SFML-3.0.2 -B /tmp/sfml-build-windows -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_SYSTEM_NAME=Windows \
        -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
        -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
        -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
        -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
        -DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32 \
        -DCMAKE_INSTALL_PREFIX=/opt/sfml-windows/sfml \
        -DSFML_BUILD_EXAMPLES=OFF \
        -DSFML_BUILD_TEST_SUITE=OFF \
        -DSFML_BUILD_DOC=OFF \
        -DBUILD_SHARED_LIBS=OFF && \
    cmake --build /tmp/sfml-build-windows -- -j$(nproc) && \
    cmake --install /tmp/sfml-build-windows && \
    rm -rf /tmp/sfml-build-windows /tmp/SFML-3.0.2 /tmp/sfml-src.zip

# Set environment variables for Windows builds
ENV SFML_WINDOWS_ROOT=/opt/sfml-windows/sfml
ENV MINGW_PREFIX=x86_64-w64-mingw32

# Convenience wrappers (expect the repo to be mounted at /app)
RUN set -eux; \
    printf '%s\n' \
        '#!/usr/bin/env bash' \
        'set -euo pipefail' \
        '' \
        'cd /app' \
        'exec ./build_tools/build.sh --linux "$@"' \
        > /usr/local/bin/linux-build-test; \
    printf '%s\n' \
        '#!/usr/bin/env bash' \
        'set -euo pipefail' \
        '' \
        'cd /app' \
        'exec ./build_tools/build.sh --linux --coverage "$@"' \
        > /usr/local/bin/linux-coverage; \
    printf '%s\n' \
        '#!/usr/bin/env bash' \
        'set -euo pipefail' \
        '' \
        'cd /app' \
        'exec ./build_tools/build.sh --windows "$@"' \
        > /usr/local/bin/windows-package; \
    printf '%s\n' \
        '#!/usr/bin/env bash' \
        'set -euo pipefail' \
        '' \
        'cd /app' \
        'exec ./build_tools/format_and_analysis.sh' \
        > /usr/local/bin/format-and-analysis; \
    printf '%s\n' \
        '#!/usr/bin/env bash' \
        'set -euo pipefail' \
        '' \
        'cd /app' \
        'exec ./build_tools/format.sh' \
        > /usr/local/bin/format-code; \
    printf '%s\n' \
        '#!/usr/bin/env bash' \
        'set -euo pipefail' \
        '' \
        'cd /app' \
        'exec ./build_tools/static_analysis.sh' \
        > /usr/local/bin/static-analysis; \
    chmod +x \
        /usr/local/bin/linux-build-test \
        /usr/local/bin/linux-coverage \
        /usr/local/bin/windows-package \
        /usr/local/bin/format-and-analysis \
        /usr/local/bin/format-code \
        /usr/local/bin/static-analysis

# Default command
CMD ["/bin/bash"] 