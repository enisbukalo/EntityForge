# Fishing Game Example

This is a small example game that consumes the installed `GameEngine` package.

## Building (recommended: via the dev container)

### Prerequisites
- The project dev container is available via `docker compose`

### Build the engine package (Windows cross-compile)

From the repo root:

```bash
docker compose run --rm dev windows-package
```

This produces a packaged install at `package_windows/` used by the example.

### Build the example (Windows cross-compile)

From the repo root:

```bash
docker compose run --rm dev ./Example/build.sh --type Debug
```

Use `--clean` if you want a full rebuild:

```bash
docker compose run --rm dev ./Example/build.sh --clean
```

## Output

- Executable: `Example/build/FishingGame.exe`
- Assets are copied next to the executable under `Example/build/assets/`
