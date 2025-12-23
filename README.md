# GameEngine

A modern C++ 2D game engine built with SFML, featuring an Entity Component System (ECS) architecture.

This README is intentionally **high level**; the detailed engine reference lives in the GitHub Wiki.

## Documentation

- Wiki Home: https://github.com/enisbukalo/2D_Game_Engine/wiki
- Getting Started: https://github.com/enisbukalo/2D_Game_Engine/wiki/Getting-Started
- Architecture: https://github.com/enisbukalo/2D_Game_Engine/wiki/Architecture
- Components: https://github.com/enisbukalo/2D_Game_Engine/wiki/Components
- Systems: https://github.com/enisbukalo/2D_Game_Engine/wiki/Systems
- API: https://github.com/enisbukalo/2D_Game_Engine/wiki/API

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

## EventBus policy (recommended)

This engine includes a world-owned, staged `EventBus` available via `World::events()`.

- **Prefer `World::events()`** for cross-system notifications and decoupled messaging.
- **Dispatch is staged**: events are delivered only when the engine calls `EventBus::pump()`.
  - `EventStage::PreFlush` is pumped immediately before `World::flushCommandBuffer()`.
  - `EventStage::PostFlush` is pumped at the end of `GameEngine::update()`.
- **ECS safety rule**: handlers that need structural changes must use `World::queue*` APIs (do not mutate stores during iteration).
- Legacy per-system subscription APIs (e.g. `Systems::SInput::subscribe`) remain temporarily for compatibility but are considered deprecated.

## Building

The recommended workflow is Docker (Linux native build + Windows cross-compile).

### Docker (recommended)

Prereqs:
- Docker
- Docker Compose

Commands:
- Build the dev image: `docker compose up -d --build`

Convenience commands (available inside the container):
- Linux build (with tests): `docker compose run --rm dev linux-build-test`
- Linux build (with coverage): `docker compose run --rm dev linux-coverage`
- Windows cross-compilation/package: `docker compose run --rm dev windows-package`
- Format + static analysis: `docker compose run --rm dev format-and-analysis`
- Format only: `docker compose run --rm dev format-code`
- Static analysis only: `docker compose run --rm dev static-analysis`

Notes:
- The dev image is a **toolchain** image; your repo is mounted at `/app` via Docker Compose.

### Build options

`build_tools/build.sh` supports:
- `-l, --linux`, `-w, --windows`, `-a, --all`
- `-t, --type TYPE` (Debug/Release)
- `-c, --clean`
- `-n, --no-tests` (Linux only)
- `-g, --coverage` (Linux only)

### Example project

The `Example/` project includes a script that builds/copies the engine package into the example.

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
- `Example/` - Example game project
- `build_tools/` - Build scripts for different platforms

## Audio Attribution

Music tracks used in this project are provided under royalty-free licenses and require attribution:

- **Sway** by Yunior Arronte  
  License: Royalty Free Music (Free with Attribution)  
  Source: [Download Link - requires attribution text from site]  
  Description: Gentle Lofi featuring piano, percussion and drums

- **Rainy Day** by Yunior Arronte  
  License: Royalty Free Music (Free with Attribution)  
  Source: [Download Link - requires attribution text from site]  
  Streaming: [Spotify](https://open.spotify.com/track/3qN47D55JWf14GQIMEDT1d) | [Apple Music](https://music.apple.com/us/album/rainy-day-single/1735587688) | [YouTube Music](https://music.youtube.com/watch?v=ZFSkcUDWlhl) | [Amazon Music](https://music.amazon.in/albums/B0CXV388LS) | [Deezer](https://deezer.page.link/K2QkQBGPpoPWnCve9)  
  Description: Lo-fi Jazz featuring jazzy piano, calming drums and bass

- **Thai motor boat** by jonny4c (Freesound)  
  License: Pixabay Content License (Free, No Attribution Required)  
  Source: https://pixabay.com/sound-effects/  
  Description: Transportation, Island, Motorboat sound effect

**Pixabay License Notes (Thai motor boat):**
- ✅ Free to use without attribution (attribution appreciated but not required)
- ✅ Can be modified or adapted
- ❌ Cannot sell or distribute as standalone content
- ❌ Cannot use in immoral or illegal ways
- ❌ Cannot use as part of trademarks or business names

**Royalty Free Music License Restrictions (Sway & Rainy Day):**
- ❌ No use in podcasts
- ❌ No audiobook creation
- ❌ No music remixing or derivative musical works
- ✅ Free to use in games and videos with attribution

## Contributing
Contributions are welcome! Please feel free to submit pull requests.

## License
MIT License