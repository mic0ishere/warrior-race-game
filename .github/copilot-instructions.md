# Warrior Race Game - AI Coding Instructions

## Project Overview
A terminal-based roguelike game written in C using ncurses library. Players navigate a fixed-size map, collect totems for points, and avoid enemies. Currently in Phase 2 with core gameplay complete; focus is on expanding features (power-ups, enemy types, obstacles).

## Architecture

### Component Structure
**Four modular C files with clear responsibilities:**

- **`main.c`**: Entry point. Initializes ncurses, creates windows, starts game loop, handles cleanup
- **`window.c/h`**: ncurses abstraction layer. Manages three windows (play area, status bar, debug output). Handles coordinate conversion between level space and screen space
- **`game.c/h`**: Core game logic. Player movement, collision detection, totem collection, enemy AI, scoring, game state management
- **`level.c/h`**: Level loading from text files. Parses CONFIG/TOTEMS/OBSTACLES/ENEMIES sections. Memory allocation for level data

### Data Flow
1. **Initialization**: `InitScreen()` → `InitAllWindows()` → `InitGame()` → load level
2. **Game Loop** (20 FPS from `FRAMES_PER_SECOND`): Input → Player move → Collision check → Enemy move → Render → Status update
3. **Shutdown**: `FinishGame()` → cleanup windows → free level memory

### Key Types (in `types.h`)
- `POSITION`: `{x, y}` coordinates in level space
- `ENEMY`: Has `pos1`, `pos2` (patrol endpoints), `current_pos`, `speed`, `direction`, `type` (HORIZONTAL/VERTICAL)
- `GAMESTATE`: Central game state - player position, score, lifes, time, level reference
- `WIN`: ncurses window wrapper with position and color info
- `LEVEL`: All level data - config (time, lifes, win_score), arrays of totems/obstacles/enemies

## Build & Development

### Building
```bash
make              # Build with gcc and ncurses
make rebuild      # Clean and build
make clean        # Remove artifacts
./warrior_race    # Run executable
```

### Configuration
- **Compiler**: gcc with flags `-Wall -Wextra -Wpedantic -Wformat-security`
- **Libraries**: ncurses (`-lncurses`)
- **Makefile**: Tracks headers as dependencies - edit `HEADERS` list when adding new .h files

## Critical Patterns & Conventions

### Level Files (`level1.txt`)
Format: INI-style sections `[CONFIG]`, `[TOTEMS]`, `[OBSTACLES]`, `[ENEMIES]`
- Coordinates are in **level space** (0,0 to WIDTH,HEIGHT)
- Enemies: Two positions define patrol path; speed is units/frame; direction `-1` starts at pos2
- Obstacles: Rectangular regions defined by two corners; collision detection uses bounding box

### Coordinate Systems
**Two coordinate systems exist - always be aware which you're using:**
- **Level space**: Game logic uses this (0–WIDTH, 0–HEIGHT)
- **Screen space**: ncurses window rendering (affected by window position, offset by `OFFX`/`OFFY`)
- `LevelToScreenCoords()` converts; use `SafePrintEntity()` for safe rendering with automatic conversion

### Window Management
- Play window: `COLS=80, ROWS=20` with `BORDER=1`, offset by `OFFX=5, OFFY=2`
- All three windows use color pairs (1–9 defined in `types.h`)
- Always call `CleanWin()` before rendering to clear previous frame

### Game Status Codes
Return `CONTINUE` (0) to keep playing, or one of: `GAME_TIMEOUT`, `OUT_OF_LIVES`, `USER_QUIT`, `WIN_POINTS`

### Common Pitfalls
- **Bounds checking**: Use `CheckWithinBounds()` for position validation against level dimensions
- **Collision**: `CheckCollision()` includes both obstacles and level boundaries
- **Memory**: Manual allocation in `LoadLevel()` - must match in `FreeLevel()` exactly
- **Timing**: Game frame rate is **20 FPS** (50ms per frame), not configurable per frame

## Common Tasks

### Adding a New Enemy Type
1. Add to `ENEMY_TYPE` enum in `types.h`
2. Extend `LoadEnemy()` parsing in `level.c` to recognize the new type
3. Extend `NextEnemyPosition()` in `game.c` with movement logic for new type

### Adding Level Features
1. Define structure in `types.h`
2. Add array to `LEVEL` struct
3. Add parsing in `LoadLevel()` and cleanup in `FreeLevel()`
4. Implement collision/interaction logic in `game.c`

### Debugging
Use debug window: Call `ShowDebug(windows.debugwin, "message")` to display on screen. Debug messages max `MAX_DEBUG_LEN` (77 chars).
Alternatively output to `stderr` for off-screen logging.

## Roadmap Features (Planned)
Check `roadmap.md` for detailed planned features

## Collaboration Notes
Project has successful experience with AI agents via PR workflow. Provide:
1. Current project phase and what's complete
2. Which file to focus on (main.c, game.c, etc.)
3. Specific refactoring or feature requests (avoid vague requests)

## C Code Quality & Style Alignment
- Use C89/C99-compatible syntax consistent with existing files; prefer explicit control flow over clever macros; avoid macros with hidden side effects.
- Keep internal linkage `static` where appropriate; minimize globals and document any unavoidable global state clearly.
- Apply defensive checks (nulls, bounds, error codes) before use; favor clear, direct logic over abstraction surprises.
- Place all new macros, constants, enums, and type definitions in `types.h`; reuse existing definitions and forbid duplicate `#define`s or parallel types; keep struct layouts and naming aligned with types.h; no magic numbers outside types.h.
- Match existing naming, include order, file layout, error-handling patterns, helper reuse, and memory management style already present in neighboring .c/.h files.
- Inspect related .c/.h files before adding code to align gameplay, rendering, and system patterns; avoid new abstractions unless the codebase already uses them; ask for clarification only if conventions conflict.
- Follow existing naming casing: functions in PascalCase, variables in lower_snake_case, macros in UPPER_SNAKE_CASE, enums/types mirroring types.h patterns; keep include order as seen in peer files (system, third-party, project headers).
- Honor ownership rules: check allocation results, free with the matching allocator, set freed pointers to NULL when appropriate, and avoid per-frame heap allocations unless unavoidable.
- Propagate errors using existing return codes and early-exit patterns; do not silently change function contracts or signatures without updating all callers and reflecting the intended behavior.
- Prefer `const` for read-only parameters, pass structs by pointer when mutating, and avoid variable-length arrays or compiler-specific extensions that break portability.
- Keep runtime constraints intact: 20 FPS game loop must not be blocked by I/O or unnecessary work; reuse existing helpers (`CheckWithinBounds`, `CheckCollision`, `LevelToScreenCoords`, `SafePrintEntity`, etc.) instead of reimplementing logic.
