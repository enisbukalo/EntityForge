# EntityForge

[![CI](https://github.com/enisbukalo/EntityForge/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/enisbukalo/EntityForge/actions/workflows/ci.yml)

A modern C++ 2D game engine built with SFML, featuring an Entity Component System (ECS) architecture.

This README is intentionally **high level**; the detailed engine reference lives in the GitHub Wiki.

## Documentation

- Wiki Home: https://github.com/enisbukalo/EntityForge/wiki
- Getting Started: https://github.com/enisbukalo/EntityForge/wiki/Getting-Started
- Architecture: https://github.com/enisbukalo/EntityForge/wiki/Architecture
- Components: https://github.com/enisbukalo/EntityForge/wiki/Components
- Systems: https://github.com/enisbukalo/EntityForge/wiki/Systems
- API: https://github.com/enisbukalo/EntityForge/wiki/API

## Highlights

- ECS-based world (`World`) with built-in components and core systems.
- Rendering via `SRenderer` (SFML backend), with camera support.
- Box2D physics via `S2DPhysics`.
- Input (`SInput`), audio (`SAudio`), particles (`SParticle`), scripting (`SScript`).

## Key runtime notes

- Fixed physics timestep: the engine enforces **60 Hz**.
- Coordinate conventions:
  - Physics (Box2D): meters, **Y-up**.
  - Rendering (SFML): pixels, **Y-down**.
  - Common scale: **100 pixels = 1 meter**.

## Building

The recommended workflow is Docker (Linux native build + Windows cross-compile).

### Docker (recommended)

Prereqs:
- Docker
- Docker Compose

Commands:
- Build the dev image: `docker compose up -d --build`
- Linux build (with tests): `docker compose exec dev ./build_tools/build.sh --linux`
- Linux build (no tests): `docker compose exec dev ./build_tools/build.sh --linux --no-tests`
- Clean build (Linux): `docker compose exec dev ./build_tools/build.sh --linux --clean`
- Windows cross-compilation: `docker compose exec dev ./build_tools/build.sh --windows`
- Build both: `docker compose exec dev ./build_tools/build.sh --all`
- Enter dev container: `docker compose exec dev /bin/bash`

### Build options

`build_tools/build.sh` supports:
- `-l, --linux`, `-w, --windows`, `-a, --all`
- `-t, --type TYPE` (Debug/Release)
- `-c, --clean`
- `-n, --no-tests` (Linux only)
- `-g, --coverage` (Linux only)

## Public API entry points

Most users interact with the engine through `GameEngine`:

- `world()` and `createEntity()` for ECS work
- `getRenderer()`, `getPhysics()`, `getInputManager()`, `getAudioSystem()`, `getParticleSystem()` for core systems

For details (components, systems, scripting lifecycle), see the Wiki links above.

## Dependencies

- C++17 or later
- CMake 3.28+
- SFML 3.0.2+
- Box2D v3.1.1
- Dear ImGui v1.91.9b + ImGui-SFML v3.0
- GoogleTest (tests)
- nlohmann/json v3.11.3

## Project Structure

The project is organized with:
- `include/` - Public headers for entities, components, systems, and utilities
- `src/` - Implementation source files
- `tests/` - Unit tests
- `build_tools/` - Build scripts for different platforms

## Rebuilding Docker Image For GHCR
```bash
GHCR_TOKEN=... bash build_tools/push_ghcr_image.sh
```

## Contributing
Contributions are welcome! Please feel free to submit pull requests.

## License
MIT License