# Warrior Race - Level Design Guide

**Target Audience**: AI agents and developers creating new game levels

**Purpose**: Comprehensive reference for level file format, validation rules, and gameplay constraints

---

## Table of Contents
1. [File Structure Overview](#file-structure-overview)
2. [Naming Conventions](#naming-conventions)
3. [Section Definitions](#section-definitions)
4. [Entity Types Reference](#entity-types-reference)
5. [Validation Rules](#validation-rules)
6. [Gameplay Constraints](#gameplay-constraints)
7. [Common Pitfalls](#common-pitfalls)
8. [Example Patterns](#example-patterns)

---

## File Structure Overview

Level files use an **INI-style format** with sections delimited by square brackets `[SECTION]`.

### Required Sections
- `[CONFIG]` - Level configuration and metadata
- `[TOTEMS]` - Collectible point items
- `[OBSTACLES]` - Static rectangular obstacles
- `[ENEMIES]` - Moving threats
- `[AGGRESSIVE_OBSTACLES]` - Timed hazards (spikes, firejets)
- `[POWERUPS]` - Collectible power-ups
- `[PASSIVE_OBSTACLES]` - Environmental effects (teleport pads, sticky floors, tall grass)

### Parsing Rules
- **Comments**: Lines starting with `#` are ignored
- **Blank lines**: Ignored
- **Section headers**: Must be on their own line, format: `[SECTION_NAME]`
- **Order**: Sections can appear in any order
- **Empty sections**: Sections can be empty (no entities defined)
- **Line length**: Maximum 256 characters (`MAX_LINE_LEN`)
- **Encoding**: Plain text (ASCII/UTF-8)

---

## Naming Conventions

### File Naming
- **Location**: All level files must be in `levels/` directory
- **Extension**: `.txt` (required for level discovery)
- **Pattern**: `<level_name>.txt`
- **Examples**: `level1.txt`, `open_field.txt`, `the_final_one.txt`
- **Level name**: Extracted from filename (without path and extension), max 63 characters

### Coordinate System
- **Origin**: Top-left corner is `(0, 0)`
- **X-axis**: Increases rightward
- **Y-axis**: Increases downward
- **Range**: `0` to `WIDTH-1` for X, `0` to `HEIGHT-1` for Y
- **Zero-indexed**: All coordinates start at 0

---

## Section Definitions

### [CONFIG]

**Purpose**: Define level metadata and parameters

**Format**: `KEY=VALUE` (one per line)

#### Required Fields

| Field | Type | Description | Constraints |
|-------|------|-------------|-------------|
| `TIME` | int | Time limit in seconds | > 0 |
| `LIFES` | int | Starting lives | > 0 |
| `WIDTH` | int | Level width | > 0, max coordinate = WIDTH-1 |
| `HEIGHT` | int | Level height | > 0, max coordinate = HEIGHT-1 |
| `START_X` | int | Player spawn X coordinate | 0 ≤ START_X < WIDTH |
| `START_Y` | int | Player spawn Y coordinate | 0 ≤ START_Y < HEIGHT |

#### Example
```
[CONFIG]
TIME=60
LIFES=3
WIDTH=30
HEIGHT=30
START_X=0
START_Y=0
```

#### Validation Notes
- All fields are required; missing fields may result in undefined behavior
- Starting position must not collide with obstacles
- Recommended: Ensure starting position is safe (not on enemy patrol path)

---

### [TOTEMS]

**Purpose**: Define collectible point items (win condition)

**Format**: `(x,y) points`

#### Syntax
```
(x,y) points_value
```

#### Fields
- **x, y**: Position coordinates (integers)
- **points_value**: Points awarded when collected (integer, typically 5-20)

#### Example
```
[TOTEMS]
# (x,y) points
(10,10) 10
(25,25) 20
(15,15) 15
```

#### Validation Notes
- Coordinates must be within level bounds: `0 ≤ x < WIDTH`, `0 ≤ y < HEIGHT`
- Totems can overlap with passive obstacles but not static obstacles
- Points contribute to `total_totems_score` (used for win condition)
- Collected totems tracked with `is_collected` flag

---

### [OBSTACLES]

**Purpose**: Define static rectangular obstacles (walls, barriers)

**Format**: `(x1,y1) (x2,y2)`

#### Syntax
```
(x1,y1) (x2,y2)
```

#### Fields
- **x1, y1**: Top-left corner of rectangle
- **x2, y2**: Bottom-right corner of rectangle

#### Example
```
[OBSTACLES]
# (x1,y1) (x2,y2)
(5,5) (8,8)           # 4x4 rectangle
(0,15) (7,16)         # Horizontal barrier
(10,15) (30,16)       # Another horizontal barrier
```

#### Validation Notes
- `x1 ≤ x2` and `y1 ≤ y2` (top-left to bottom-right)
- All corners must be within level bounds
- Player cannot walk through obstacles
- Enemies respect obstacles in movement
- Obstacles can form complex maze patterns

---

### [ENEMIES]

**Purpose**: Define moving threats that kill the player on contact

**Enemy Types**: PATROL, RANDOM, CHASER, TELEPORTER

#### Common Format
```
TYPE (x1,y1) (x2,y2) (start_x,start_y) speed [extra_params]
```

#### Common Fields
- **TYPE**: Enemy type string (PATROL, RANDOM, CHASER, TELEPORTER)
- **x1, y1**: Movement boundary top-left
- **x2, y2**: Movement boundary bottom-right
- **start_x, start_y**: Initial spawn position
- **speed**: Movement speed (1-5, validated and clamped)

#### PATROL Enemy
**Behavior**: Moves back and forth between two endpoints

**Format**:
```
PATROL (x1,y1) (x2,y2) (start_x,start_y) speed
```

**Type Detection**:
- Horizontal: `y1 == y2` (same Y coordinate)
- Vertical: `x1 == x2` (same X coordinate)

**Example**:
```
PATROL (0,10) (10,10) (5,10) 1     # Horizontal patrol
PATROL (20,20) (20,25) (20,20) 3   # Vertical patrol
```

**Notes**:
- Start position must be on the patrol line
- Speed affects frames per movement

#### RANDOM Enemy
**Behavior**: Wanders randomly within bounded area

**Format**:
```
RANDOM (x1,y1) (x2,y2) (start_x,start_y) speed
```

**Example**:
```
RANDOM (20,0) (29,20) (25,10) 2
```

**Notes**:
- Moves randomly but respects obstacles
- Bounded by `(x1,y1)` to `(x2,y2)` area
- Cannot move outside boundary

#### CHASER Enemy
**Behavior**: Pursues player when within chase radius, sleeps otherwise

**Format**:
```
CHASER (x1,y1) (x2,y2) (start_x,start_y) speed chase_radius
```

**Extra Parameters**:
- **chase_radius**: Distance to detect and chase player (integer, default: 10)

**Example**:
```
CHASER (20,0) (29,10) (25,2) 3 8    # Chase radius = 8
```

**Notes**:
- Displays as blue when sleeping, bright red when chasing
- Uses Manhattan distance for detection
- Chases directly toward player (smart pathfinding)

#### TELEPORTER Enemy
**Behavior**: Randomly teleports within bounded area every 3 seconds

**Format**:
```
TELEPORTER (x1,y1) (x2,y2) (start_x,start_y) speed
```

**Example**:
```
TELEPORTER (0,20) (10,29) (0,25) 1
```

**Notes**:
- Teleport interval: 3 seconds (`TELEPORTER_INTERVAL_FRAMES`)
- Flashes magenta 0.66 seconds before teleporting (`TELEPORTER_WARNING_FRAMES`)
- Teleports to random valid position within bounds

#### Enemy Speed Reference
| Speed | Movement Frequency |
|-------|-------------------|
| 1 | Every 20 frames (1 second) |
| 2 | Every 10 frames (0.5 seconds) |
| 3 | Every ~6 frames (~0.3 seconds) |
| 4 | Every 5 frames (0.25 seconds) |
| 5 | Every 4 frames (0.2 seconds) |

**Formula**: Movement every `20 / speed` frames (at 20 FPS)

#### Enemy Direction
- Automatically initialized to `DIR_FORWARD` (1) or `DIR_BACKWARD` (-1) randomly on load
- Direction changes when reaching endpoints (patrols)

---

### [AGGRESSIVE_OBSTACLES]

**Purpose**: Define timed hazards that activate at intervals

**Types**: SPIKE, FIREJET

#### SPIKE
**Behavior**: Harms player when stepping onto spike from specified direction

**Format**:
```
SPIKE (x,y) direction interval
```

**Fields**:
- **x, y**: Spike position
- **direction**: Entry direction blocked (UP, DOWN, LEFT, RIGHT)
- **interval**: Seconds between activations (float, e.g., `1.5`, `3`)

**Example**:
```
[AGGRESSIVE_OBSTACLES]
SPIKE (12,5) UP 3         # Blocks upward entry, activates every 3 seconds
SPIKE (10,2) LEFT 1.5     # Blocks leftward entry, activates every 1.5 seconds
```

**Notes**:
- Spike only blocks movement when moving INTO the spike direction
- Example: `SPIKE (x,y) UP` blocks player moving up (from y+1 to y)
- Active duration: 1 second (`OBSTACLE_ACTIVE_DURATION_FRAMES`)
- Interval converted to frames: `interval * FRAMES_PER_SECOND`

#### FIREJET
**Behavior**: Shoots fire in straight line at intervals

**Format**:
```
FIREJET (x,y) direction interval length
```

**Fields**:
- **x, y**: Firejet source position
- **direction**: Fire travel direction (UP, DOWN, LEFT, RIGHT)
- **interval**: Seconds between activations (float)
- **length**: Distance fire travels (integer, in tiles)

**Example**:
```
FIREJET (5,10) RIGHT 2 7     # Shoots right 7 tiles every 2 seconds
FIREJET (15,25) UP 3 5       # Shoots up 5 tiles every 3 seconds
```

**Notes**:
- Firejet source is a solid obstacle (cannot walk through)
- Fire path damages player if standing on it when active
- Active duration: 1 second
- Fire path respects obstacles (stopped by walls)

#### Direction Reference
| Direction | Effect |
|-----------|--------|
| UP | Decreases Y coordinate |
| DOWN | Increases Y coordinate |
| LEFT | Decreases X coordinate |
| RIGHT | Increases X coordinate |

---

### [POWERUPS]

**Purpose**: Define collectible power-ups with temporary or permanent effects

**Types**: SHIELD, DOUBLE, LIFE, FREEZE

#### Common Format
```
TYPE (x,y)
```

#### Power-up Types

| Type | Symbol | Effect | Duration |
|------|--------|--------|----------|
| **SHIELD** | [S] | Negates next enemy collision | One-time use |
| **DOUBLE** | [D] | Doubles totem point values | 10 seconds |
| **LIFE** | [+] | Grants one extra life | Instant |
| **FREEZE** | [F] | Stops all enemy movement | 5 seconds |

#### Example
```
[POWERUPS]
# type (x,y)
SHIELD (3,3)
DOUBLE (18,18)
LIFE (27,27)
FREEZE (15,5)
```

#### Notes
- Power-ups tracked with `is_collected` flag
- Maximum 4 active timed power-ups (`MAX_ACTIVE_POWERUPS`)
- Shield provides one-time protection, removed after use
- Double points multiplies totem values by 2
- Time freeze prevents enemy movement but not aggressive obstacles

---

### [PASSIVE_OBSTACLES]

**Purpose**: Define environmental effects and area-based obstacles

**Types**: TELEPORT, STICKY, GRASS

#### TELEPORT (Teleport Pad)
**Behavior**: Instantly transports player to linked pad

**Format**:
```
TELEPORT (x,y) (x,y) my_id target_id
```

**Fields**:
- **First (x,y)**: This pad's position
- **Second (x,y)**: Same as first (format requirement)
- **my_id**: This pad's unique identifier (integer, starting from 0)
- **target_id**: Destination pad ID to teleport to

**Example**:
```
[PASSIVE_OBSTACLES]
TELEPORT (2,28) (2,28) 0 1    # Pad 0 → teleports to Pad 1
TELEPORT (28,2) (28,2) 1 0    # Pad 1 → teleports to Pad 0
```

**Notes**:
- Cooldown: 2 seconds (`TELEPORT_PAD_COOLDOWN_FRAMES`) after use
- IDs must match between linked pads
- Can create one-way or two-way teleports
- Target pad must exist in level

#### STICKY (Sticky Floor)
**Behavior**: Slows player movement (4x slower)

**Format**:
```
STICKY (x1,y1) (x2,y2)
```

**Fields**:
- **x1, y1**: Top-left corner of area
- **x2, y2**: Bottom-right corner of area

**Example**:
```
STICKY (15,8) (17,11)     # Rectangular sticky area
```

**Notes**:
- Player moves every 4 frames instead of every frame (`STICKY_FLOOR_SLOWDOWN`)
- Multiple sticky areas can exist
- Effect applies when player position is within bounds

#### GRASS (Tall Grass)
**Behavior**: Reduces player's visibility range

**Format**:
```
GRASS (x1,y1) (x2,y2)
```

**Fields**:
- **x1, y1**: Top-left corner of area
- **x2, y2**: Bottom-right corner of area

**Example**:
```
GRASS (20,17) (29,23)     # Rectangular grass area
```

**Notes**:
- Visibility range: 2 tiles around player (`TALL_GRASS_VISION_RANGE`)
- Gradual transition: 10 frames to fade in/out (`TALL_GRASS_TRANSITION_FRAMES`)
- Multiple grass areas can exist
- Does not affect enemy movement or behavior

---

## Validation Rules

### Coordinate Validation
1. **Bounds checking**: All entity coordinates must satisfy:
   - `0 ≤ x < WIDTH`
   - `0 ≤ y < HEIGHT`
2. **Rectangle validation**: For rectangular areas:
   - `x1 ≤ x2` and `y1 ≤ y2`
3. **Starting position**: Must not collide with static obstacles

### Enemy Validation
1. **Speed clamping**: Speed automatically clamped to range [1, 5]
   - Warning printed to stderr if clamping occurs
2. **Start position**: Validated with `CheckWithinBounds()`
   - If invalid, defaults to `pos1` (top-left of bounds)
3. **Patrol type detection**:
   - Horizontal: Requires `y1 == y2`
   - Vertical: Requires `x1 == x2`
   - Invalid patrol lines result in `ENEMY_TYPE_UNKNOWN`
4. **Chase radius**: Defaults to 10 if not specified (CHASER only)

### Aggressive Obstacle Validation
1. **Interval conversion**: Seconds converted to frames
   - Formula: `interval * 20` (at 20 FPS)
   - Supports float values (e.g., `1.5` seconds = 30 frames)
2. **Direction parsing**: Must be exact match (case-sensitive)
   - Valid: UP, DOWN, LEFT, RIGHT
   - Invalid strings default to first valid direction

### Memory Allocation
- All entity arrays dynamically allocated using `realloc()`
- Allocation failure causes level load to fail (returns NULL)
- Level cleanup handled by `FreeLevel()` function

---

## Gameplay Constraints

### Movement and Collision
1. **Player movement**: 1 tile per frame (20 FPS = 20 tiles/second)
   - Slowed to 1 tile per 4 frames on sticky floors
2. **Enemy movement**: Based on speed parameter (see Enemy Speed Reference)
3. **Collision detection order**:
   - Static obstacles → Aggressive obstacles → Level boundaries
   - Post-move: Enemy collision → Fire paths → Active spikes
4. **Directional spikes**: Only block movement when entering from spike direction
   - Example: Spike with direction UP blocks movement from (x, y+1) to (x, y)

### Scoring System
1. **Totem collection**: Points added to score
   - Doubled if DOUBLE power-up active
2. **Final score calculation**:
   ```
   final_score = totems_score 
                 + (lives_left * FINAL_SCORE_LIFE_BONUS)
                 + (time_left * FINAL_SCORE_TIME_BONUS)
   ```
   - `FINAL_SCORE_LIFE_BONUS = 20`
   - `FINAL_SCORE_TIME_BONUS = 1`

### Win/Loss Conditions
- **Win**: Collect all totems before time expires (`WIN_POINTS` status)
- **Loss conditions**:
  - Time expires: `GAME_TIMEOUT`
  - All lives lost: `OUT_OF_LIFES`
  - Player quits: `USER_QUIT`

### Life Loss System
1. **Death causes**:
   - Enemy collision (4 types)
   - Active spike collision
   - Active firejet path
2. **Respawn**: Player teleported back to `START_X, START_Y`
3. **Shield protection**: SHIELD power-up negates one death

### Timing
- **Game loop**: 20 FPS (`FRAMES_PER_SECOND`)
- **Frame time**: 50ms per frame (`FRAME_TIME_MS`)
- **Time tracking**: `frames_elapsed / FRAMES_PER_SECOND = time_elapsed`

---

## Common Pitfalls

### ❌ INCORRECT Examples

#### 1. Out of Bounds Coordinates
```
[CONFIG]
WIDTH=30
HEIGHT=30

[TOTEMS]
(30,30) 10    # ❌ Max coordinate is 29 (WIDTH-1)
```

**Fix**:
```
(29,29) 10    # ✓ Within bounds [0, 29]
```

#### 2. Invalid Rectangle Definition
```
[OBSTACLES]
(10,10) (5,5)    # ❌ Top-right before bottom-left
```

**Fix**:
```
(5,5) (10,10)    # ✓ Top-left to bottom-right
```

#### 3. Invalid Patrol Line
```
[ENEMIES]
PATROL (0,10) (10,15) (5,12) 2    # ❌ Neither horizontal nor vertical
```

**Fix**:
```
PATROL (0,10) (10,10) (5,10) 2    # ✓ Horizontal patrol (same Y)
# OR
PATROL (5,10) (5,15) (5,12) 2     # ✓ Vertical patrol (same X)
```

#### 4. Missing Required Config Fields
```
[CONFIG]
TIME=60
LIFES=3
# ❌ Missing WIDTH, HEIGHT, START_X, START_Y
```

**Fix**:
```
[CONFIG]
TIME=60
LIFES=3
WIDTH=30
HEIGHT=30
START_X=0
START_Y=0
```

#### 5. Invalid Enemy Speed
```
[ENEMIES]
PATROL (0,10) (10,10) (5,10) 10    # ❌ Speed > 5
```

**Result**: Automatically clamped to 5 (warning printed to stderr)

#### 6. Missing Firejet Length
```
[AGGRESSIVE_OBSTACLES]
FIREJET (5,10) RIGHT 2    # ❌ Missing length parameter
```

**Fix**:
```
FIREJET (5,10) RIGHT 2 7    # ✓ Length = 7 tiles
```

#### 7. Mismatched Teleport Pad Coordinates
```
[PASSIVE_OBSTACLES]
TELEPORT (2,28) (3,29) 0 1    # ❌ Coordinates don't match
```

**Fix**:
```
TELEPORT (2,28) (2,28) 0 1    # ✓ Both coordinates identical
```

#### 8. Starting Position on Obstacle
```
[CONFIG]
START_X=5
START_Y=5

[OBSTACLES]
(5,5) (8,8)    # ❌ Player spawns inside obstacle
```

**Fix**: Choose different starting position or move obstacle

---

## Example Patterns

### Simple Corridor Level
```
[CONFIG]
TIME=45
LIFES=3
WIDTH=25
HEIGHT=10
START_X=0
START_Y=5

[TOTEMS]
(24,5) 20

[OBSTACLES]
(0,0) (24,3)     # Top wall
(0,7) (24,9)     # Bottom wall

[ENEMIES]
PATROL (5,5) (20,5) (10,5) 2

[AGGRESSIVE_OBSTACLES]
FIREJET (10,4) DOWN 2 3

[POWERUPS]
SHIELD (15,5)

[PASSIVE_OBSTACLES]
```

### Multi-Room Maze
```
[CONFIG]
TIME=90
LIFES=5
WIDTH=30
HEIGHT=30
START_X=1
START_Y=1

[TOTEMS]
(5,5) 10
(15,15) 15
(25,25) 20

[OBSTACLES]
# Room dividers
(10,0) (10,9)
(10,11) (10,19)
(10,21) (10,29)
(20,0) (20,9)
(20,11) (20,29)

[ENEMIES]
PATROL (0,10) (9,10) (5,10) 1
CHASER (11,0) (19,9) (15,5) 2 8
RANDOM (21,0) (29,29) (25,15) 3

[AGGRESSIVE_OBSTACLES]
SPIKE (10,10) RIGHT 2
SPIKE (10,20) RIGHT 2
FIREJET (20,10) LEFT 3 5

[POWERUPS]
SHIELD (2,2)
DOUBLE (12,12)
FREEZE (22,22)

[PASSIVE_OBSTACLES]
TELEPORT (5,28) (5,28) 0 1
TELEPORT (25,2) (25,2) 1 0
STICKY (11,21) (19,28)
GRASS (0,11) (9,19)
```

### Chase Arena
```
[CONFIG]
TIME=60
LIFES=2
WIDTH=20
HEIGHT=20
START_X=10
START_Y=10

[TOTEMS]
(0,0) 15
(19,0) 15
(0,19) 15
(19,19) 15

[OBSTACLES]
# Center obstacle
(8,8) (11,11)

[ENEMIES]
CHASER (0,0) (19,19) (5,5) 3 10
CHASER (0,0) (19,19) (14,14) 3 10

[AGGRESSIVE_OBSTACLES]
SPIKE (9,7) DOWN 1.5
SPIKE (10,7) DOWN 1.5

[POWERUPS]
FREEZE (10,10)
SHIELD (2,2)

[PASSIVE_OBSTACLES]
```

---

## Testing and Validation

### Pre-Release Checklist
1. ✓ All CONFIG fields present and positive
2. ✓ All coordinates within [0, WIDTH-1] × [0, HEIGHT-1]
3. ✓ Starting position not colliding with obstacles
4. ✓ Rectangles defined top-left to bottom-right
5. ✓ Enemy speeds in range [1, 5]
6. ✓ Patrol enemies have valid horizontal/vertical lines
7. ✓ Teleport pad IDs properly linked
8. ✓ All sections present (even if empty)
9. ✓ File saved in `levels/` directory with `.txt` extension
10. ✓ Level is winnable (totems reachable, time sufficient)

### Testing Commands
```bash
# Build game
make

# Run game
./warrior_race

# Select your level from menu
# Play through to verify:
# - All totems reachable
# - No unfair enemy placements
# - Time limit appropriate
# - Power-ups accessible
```

### Common Testing Issues
- **Level not appearing**: Check file is in `levels/` and has `.txt` extension
- **Parse errors**: Check stderr output for warnings about unknown types
- **Crashes**: Check for out-of-bounds coordinates or invalid rectangles
- **Unwinnable**: Ensure totems not blocked by obstacles or unreachable

---

## Advanced Topics

### Performance Considerations
- Large numbers of entities (>50) may impact performance
- Consider enemy speed distribution to maintain difficulty
- Avoid overlapping too many grass/sticky areas

### Difficulty Balancing
- **Easy**: Low enemy speed (1-2), long time limit, many lives
- **Medium**: Mixed speeds (2-3), moderate time, 3-5 lives
- **Hard**: High speeds (3-5), tight time limits, 2-3 lives
- **Expert**: Multiple chasers, teleporters, aggressive obstacles, minimal time

### Level Design Philosophy
1. **Progressive difficulty**: Start safe, increase challenge toward end
2. **Risk-reward**: Place valuable totems near hazards
3. **Player choice**: Multiple paths to objectives
4. **Power-up placement**: Strategic locations for maximum impact
5. **Testing**: Playtest multiple times to ensure fairness

---

## Quick Reference Card

### Entity Syntax Summary
```
# CONFIG
KEY=VALUE

# TOTEMS
(x,y) points

# OBSTACLES
(x1,y1) (x2,y2)

# ENEMIES
PATROL (x1,y1) (x2,y2) (sx,sy) speed
RANDOM (x1,y1) (x2,y2) (sx,sy) speed
CHASER (x1,y1) (x2,y2) (sx,sy) speed radius
TELEPORTER (x1,y1) (x2,y2) (sx,sy) speed

# AGGRESSIVE_OBSTACLES
SPIKE (x,y) direction interval
FIREJET (x,y) direction interval length

# POWERUPS
SHIELD (x,y)
DOUBLE (x,y)
LIFE (x,y)
FREEZE (x,y)

# PASSIVE_OBSTACLES
TELEPORT (x,y) (x,y) my_id target_id
STICKY (x1,y1) (x2,y2)
GRASS (x1,y1) (x2,y2)
```

### Coordinate Cheat Sheet
- Origin: (0, 0) = top-left
- Max X: WIDTH - 1
- Max Y: HEIGHT - 1
- X increases: →
- Y increases: ↓

### Speed vs. Frequency
| Speed | Tiles/Second |
|-------|--------------|
| 1 | 1 |
| 2 | 2 |
| 3 | ~3.3 |
| 4 | 5 |
| 5 | 5 |

---

## Appendix: Parser Implementation Notes

### Level Loading Process (`LoadLevel`)
1. Allocate `LEVEL` structure
2. Extract level name from filename
3. Open file for reading
4. Initialize all entity arrays to NULL
5. Read line by line:
   - Skip comments and blank lines
   - Detect section headers
   - Parse entities based on current section
   - Dynamically allocate arrays with `realloc()`
6. Close file and return level pointer

### Entity Addition (`AddLevelEntity`)
- Generic function for all entity types
- Grows array by one entry using `realloc()`
- Copies new entity data into array
- Returns NULL on allocation failure

### Cleanup (`FreeLevel`)
- Frees all entity arrays
- Frees level structure
- NULL-safe (can pass NULL pointer)

### Error Handling
- Warnings printed to stderr for:
  - Speed clamping
  - Unknown entity types
  - Invalid patrol lines
- Critical errors return NULL:
  - File open failure
  - Memory allocation failure
  - Missing sections (depending on context)

---

**Document Version**: 1.0  
**Last Updated**: 2026-01-08  
**Game Version**: Phase 2 (with power-ups and passive obstacles)  
**Maintained by**: Warrior Race Development Team
