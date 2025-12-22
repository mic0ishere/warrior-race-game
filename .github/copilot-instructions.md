# Warrior Race Game - AI Coding Instructions

## Project Overview
A terminal-based roguelike game written in C using ncurses library.  Players navigate dynamically-sized maps, collect totems for points, avoid multiple enemy types, and overcome aggressive obstacles. Currently completing Phase 2 with core gameplay and most features implemented; remaining work focuses on power-ups and passive obstacles.

## Architecture

### Component Structure
**Modular C files with clear responsibilities:**

- **`main.c`**: Entry point. Initializes ncurses, manages main menu loop, creates game windows, orchestrates game sessions, handles cleanup
- **`window.c/h`**: ncurses abstraction layer. Manages three windows (play area, status bar, debug output). Handles coordinate conversion between level space and screen space, rendering all game entities, alerts, and status displays
- **`game.c/h`**: Core game logic. Player movement, collision detection (static + directional obstacles), totem collection, enemy AI (4 types), aggressive obstacle updates, life loss handling, scoring, game state management, main game loop
- **`level.c/h`**: Level loading from text files. Parses CONFIG/TOTEMS/OBSTACLES/ENEMIES sections. Memory allocation for level data
- **`types.h`**: Central header defining all key types, constants, enums, and macros used across modules
- **`menu.c/h`**: Menu system. Main menu (new game/ranking/exit), level selector with scrolling support, ranking display with level switching, level file discovery
- **`ranking.c/h`**: Scoring and persistence. Final score calculation with bonuses, ranking file I/O (plaintext format), per-level ranking retrieval and sorting

### Data Flow
1. **Initialization**: `InitScreen()` → Main menu loop → Level selector → `InitAllWindows()` → `LoadLevel()` → `InitGame()`
2. **Game Loop** (20 FPS from `FRAMES_PER_SECOND`): 
   - Input handling (movement/pause/quit)
   - Collision check → Player move
   - `MoveEnemies()` (speed-based frame throttling)
   - `CheckPlayerEnemyCollision()`
   - `UpdateAggressiveObstacles()` (interval-based activation)
   - `CollectTotems()`
   - Render all entities with `RenderPlayArea()`
   - Update status and debug windows
   - Check win/loss conditions
   - Handle life loss (alert, delay, respawn)
3. **Shutdown**: `FinishGame()` → Save ranking → Display final score → Cleanup windows → Free level memory → Return to main menu

### Key Types (in `types.h`)
- `POSITION`: `{x, y}` coordinates in level space
- `ENEMY`: Has `pos1`, `pos2` (patrol endpoints), `current_pos`, `speed`, `direction`, `type` (HORIZONTAL/VERTICAL/RANDOM WALKER, CHASER, TELEPORTER)
- `GAMESTATE`: Central game state - player position, score, lifes, time, level reference
- `WIN`: ncurses window wrapper with position and color info
- `LEVEL`: All level data - config (time, lifes, etc), arrays of totems/obstacles/enemies
- `OBSTACLE`: Rectangular area with top-left and bottom-right positions
- `AGGRESSIVE_OBSTACLE`: Obstacle with activation interval and active state, types: SPIKE, FIREJET

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

### Level Files (`levels/*. txt`)

**Location**: All level files must be in `levels/` directory with `.txt` extension

**Format**: INI-style sections with specific syntax per entity type

```text
[CONFIG]
TIME=60              # Time limit in seconds
LIFES=3              # Starting lives
WIDTH=30             # Level width (max X coordinate + 1)
HEIGHT=30            # Level height (max Y coordinate + 1)
START_X=0            # Player spawn X
START_Y=0            # Player spawn Y

[TOTEMS]
# (x,y) points
(10,10) 10           # Totem at position (10,10) worth 10 points
(15,15) 15

[OBSTACLES]
# (x1,y1) (x2,y2) - rectangular static obstacles
(5,5) (8,8)          # Rectangle from top-left to bottom-right

[ENEMIES]
# type          (x1,y1) (x2,y2) (start_x,start_y) speed [extra_params]
PATROL (0,10) (10,10) (5,10) 1                    # Horizontal patrol
PATROL (20,20) (20,25) (20,20) 3                  # Vertical patrol
RANDOM (20,0) (29,20) (25,10) 2                   # Random walker in bounds
CHASER (20,0) (29,10) (25,2) 3 8                  # Chase radius = 8
TELEPORTER (0,20) (10,29) (0,25) 1                # Teleports in bounds

[AGGRESSIVE_OBSTACLES]
# SPIKE (x,y) direction interval
SPIKE (12,5) UP 3           # Activates every 3 seconds, harms from UP direction
SPIKE (10,2) LEFT 1. 5       # Activates every 1.5 seconds

# FIREJET (x,y) direction interval length
FIREJET (5,10) RIGHT 2 7    # Shoots 7 cells right, every 2 seconds
FIREJET (15,25) UP 3 5      # Shoots 5 cells up, every 3 seconds
```

**Parsing Details**:
- Comments start with `#`
- Coordinates are in **level space** (0-indexed)
- Enemy speed:  1-5 (validated and clamped in `LoadEnemy()`)
- Enemy direction:  Randomly set to `DIR_FORWARD(1)` or `DIR_BACKWARD(-1)` on load
- Aggressive obstacle intervals: In seconds, converted to frames on load

### Coordinate Systems
**Two coordinate systems exist - always be aware which you're using:**
- **Level space**: Game logic coordinate system
  - Origin (0,0) at top-left
  - X increases right, Y increases down
  - Range: 0 to WIDTH-1, 0 to HEIGHT-1
  - Used in:  collision detection, entity positions, level files
  
- **Screen space**: ncurses window coordinate system
  - Relative to window's top-left corner
  - Centered on player with camera following
  - Converted via `LevelToScreenCoords()`: 
    - `screen_x = center_x + (level_x - player_x)`
    - `screen_y = center_y + (player_y - level_y)` ← Note Y is inverted!


**Critical Functions**:
```c
void LevelToScreenCoords(WIN *W, int level_x, int level_y, GAMESTATE *state,
                         int *screen_x, int *screen_y);
void SafePrintEntity(WIN *W, int level_x, int level_y, const char *symbol,
                     GAMESTATE *state, int color_pair);
```
Always use `SafePrintEntity()` for rendering - it handles conversion and bounds checking.

### Window Management while in-game
**Three Windows**:
1. **Play window**: `COLS=80, ROWS=20` - main game area
2. **Status window**:  `COLS=80, STAT_ROWS=5` - score, time, lives, controls
3. **Debug window**: `COLS=80, ROWS=3` - debug messages (only if `DEBUG` defined)

**Screen Layout**:
```
    OFFX=5
    ↓
OFFY=2 → ┌─────────────────────────┐
         │     Play Window         │ ← 20 rows
         │                         │
         ├─────────────────────────┤
         │    Status Window        │ ← 5 rows
         ├─────────────────────────┤
         │    Debug Window         │ ← 3 rows (if DEBUG)
         └─────────────────────────┘
```

### Game Status Codes
Return `CONTINUE` (0) to keep playing, or one of: `GAME_TIMEOUT`, `OUT_OF_LIVES`, `USER_QUIT`, `WIN_POINTS`

### Common Pitfalls
- **Bounds checking**: Use `CheckWithinBounds()` for position validation against level dimensions
- **Collision**: `CheckCollision()` includes both obstacles and level boundaries
- **Memory**: Manual allocation in `LoadLevel()` - must match in `FreeLevel()` exactly
- **Timing**: Game frame rate is **20 FPS** (50ms per frame), not configurable per frame

### Collision Detection
**Two types implemented**: 

1. **Static Collision** (`CheckCollision()`): Returns FALSE if move is invalid
   - Level boundary check
   - Static obstacles (rectangular bounds)
   - Firejet source positions (can't walk through)
   - Directional spikes (only block when moving INTO spike direction)

2. **Post-Move Collision** (checked after player moves):
   - Enemy collision → `CheckPlayerEnemyCollision()`
   - Fire paths → `IsOnFirePath()`
   - Active spikes → `UpdateAggressiveObstacles()`

### Menu System
**Main Menu** (`ShowMainMenu()`):
- Three options: New Game (0), View Ranking (1), Exit (2)
- Navigation:  W/S keys or number keys (1-3)
- Space to confirm, Q to quit
- Returns selected option as integer

**Level Selector** (`ShowLevelSelector()`):
- Scans `levels/` directory for `.txt` files
- Sorts alphabetically
- Scrolling support for >visible_items levels
- Shows "...  X more levels" indicator when scrolled
- Returns `LEVEL*` (loaded level) or `NULL` if cancelled

**Ranking Viewer** (`ShowRanking()`):
- Displays top 10 scores per level
- A/D keys to switch between levels
- Shows:  Rank, Final Score, Totems, Lives, Time Left
- Persistent across sessions (stored in `rankings.dat`)

### Ranking System
**File Format** (`rankings.dat`): Plaintext, pipe-delimited
```
level1|150|30|2|10|1701234567
level_name|final_score|totems_score|lives_left|time_left|timestamp
```

**Save Process** (`SaveRanking()`):
1. Create new `RANKING_ENTRY` with calculated score
2. Load existing rankings for current level (max 10)
3. Add new entry, sort by score (descending)
4. Keep top 10 for this level
5. Write back to file (this level's top 10 + all other levels)

**Retrieval** (`GetLevelRankings()`):
- Loads all rankings from file
- Filters by `level_name`
- Sorts by `final_score` descending
- Returns up to `max_entries`

## Common Tasks

### Adding a New Enemy Type
1. Add to `ENEMY_TYPE` enum in `types.h`
2. Add fields to `ENEMY` struct if needed*(like `chase_radius` for CHASER)
2. Extend `LoadEnemy()` parsing in `level.c` to recognize the new type
4. Implement movement logic in `MoveEnemies()` in `game.c`
5. Add visual feedback in `RenderPlayArea()` in `window.c`
6. Update level file documentation and add example levels

### Adding a New Obstacle Type
1. Add enum in `types.h`
2. Add fields to `AGGRESSIVE_OBSTACLE` struct if needed
3. Create loader function in `level.c` (following `LoadAggressiveObstacle()` pattern)
4. Update parser in `LoadLevel()`
5. Implement behavior in `game.c` (e.g., in `UpdateAggressiveObstacles()` or create a new function)
6. Add rendering in `RenderPlayArea()` in `window.c`

### Adding Other Level Features
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

## Coding Style & Best Practices

### C Style Guidelines
- **Naming**: `PascalCase` for types, `camelCase` for variables, `UPPER_CASE` for macros
- **Bracing**: Opening brace on same line for functions, control structures
- **Indentation**: 2 spaces (no tabs)
- **Structs**: Always use `typedef struct { ... } NAME;` pattern
- **Enums**: Place all type-specific enums in `types.h` before structs that use them
- **Constants**: Place macros in `types.h` and typed constants as `const` in .c files

### Memory Safety
- **Always check return values** from `malloc()`, `realloc()`, `fopen()`
- **Use `strncpy()` with size-1** and manual null termination
- **Validate array indices** before access
- **Free in reverse order** of allocation
- **NULL check before free** (though `free(NULL)` is safe)

### ncurses Best Practices
- **Never use raw coordinates** - always use `SafePrintEntity()` for game entities
- **Refresh after drawing** - `wrefresh(window)` to update display
- **Check window size** - ensure content fits in `W->rows` and `W->cols`
- **Clean before redraw** - use `CleanWin()` to clear window contents
- **Use color pairs** - never assume color support, check if already initialized in `window.c`

## Code Quality & Style Alignment
- **Match existing style**: naming, include order, file layout, error-handling patterns, helper reuse, and memory management style already present in neighboring .c/.h files.
- **Inspect related .c/.h files before adding code**: align generated code with gameplay, rendering, and system patterns; avoid new abstractions unless the codebase already uses them or they are strictly necessary.
- **Follow existing naming casing**: functions in PascalCase, variables in lower_snake_case, macros in UPPER_SNAKE_CASE, enums/types mirroring types.h patterns; keep include order as seen in peer files (system, third-party, project headers).
- **Keep runtime constraints intact**: 20 FPS game loop must not be blocked by I/O or unnecessary work; reuse existing helpers (`CheckWithinBounds`, `CheckCollision`, `LevelToScreenCoords`, `SafePrintEntity`, etc.) instead of reimplementing logic.
- **IMPORTANT: Keep function length to a maximum of 1024 bytes** (hard limit: 1200 bytes) to ensure maintainability and readability.