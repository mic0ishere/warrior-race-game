# Passive Obstacles Documentation

## Overview

Passive obstacles are environmental features that affect player behavior when entered. Unlike aggressive obstacles (spikes, fire jets), passive obstacles are not harmful but modify movement, positioning, or visibility.

## Obstacle Types

### 1. Teleport Pads (Symbol: `P`)
- **Effect**: Instantly transports the player to another teleport pad
- **Behavior**: 
  - Teleportation occurs immediately when player enters a pad
  - Each pad is linked to another pad via `teleport_target_id`
  - 2-second cooldown prevents infinite teleport loops
  - Teleports to the center of the target pad
- **Visual**: Purple/magenta colored `P` symbol
- **Level Format**:
  ```
  TELEPORT (x1,y1) (x2,y2) target_id
  ```
  Example:
  ```
  TELEPORT (5,15) (5,15) 1    # Pad 0 - teleports to pad 1
  TELEPORT (20,5) (20,5) 0    # Pad 1 - teleports to pad 0
  ```

### 2. Sticky Floor (Symbol: `~`)
- **Effect**: Slows down player movement
- **Behavior**:
  - Player must press movement key twice to move one tile
  - Slowdown applies only while standing on sticky floor
  - Normal movement resumes immediately upon leaving
  - Does not affect enemies
- **Visual**: Yellow colored `~` symbol
- **Level Format**:
  ```
  STICKY (x1,y1) (x2,y2)
  ```
  Example:
  ```
  STICKY (10,15) (14,18)    # Rectangular sticky area
  ```

### 3. Tall Grass (Symbol: `,`)
- **Effect**: Reduces player vision
- **Behavior**:
  - Limits visibility to 2 tiles in all directions from player
  - Affects rendering of all entities (totems, enemies, obstacles)
  - Normal visibility restored when player exits grass
  - Creates a fog-of-war effect
- **Visual**: Green colored `,` symbol
- **Level Format**:
  ```
  GRASS (x1,y1) (x2,y2)
  ```
  Example:
  ```
  GRASS (15,25) (20,28)    # Rectangular grass area
  ```

## Level File Format

Passive obstacles are defined in the `[PASSIVE_OBSTACLES]` section of level files:

```
[PASSIVE_OBSTACLES]
# Teleport pads - must be paired with target_id
TELEPORT (x1,y1) (x2,y2) target_id

# Sticky floor - can be single tile or area
STICKY (x1,y1) (x2,y2)

# Tall grass - can be single tile or area  
GRASS (x1,y1) (x2,y2)
```

### Complete Example

```
[PASSIVE_OBSTACLES]
# Teleport pad pair (indices 0 and 1)
TELEPORT (5,8) (6,9) 1      # Pad 0 teleports to pad 1
TELEPORT (35,8) (36,9) 0    # Pad 1 teleports to pad 0

# Sticky floor areas
STICKY (10,15) (14,18)
STICKY (25,5) (29,8)

# Tall grass areas
GRASS (15,25) (20,28)
GRASS (30,12) (38,18)
```

## Implementation Details

### Data Structures

**PASSIVE_OBSTACLE** struct:
- `pos1, pos2`: Rectangular bounds (can be same for single tile)
- `type`: PASSIVE_TELEPORT_PAD, PASSIVE_STICKY_FLOOR, or PASSIVE_TALL_GRASS
- `teleport_target_id`: Index of destination pad (only for teleports)
- `teleport_cooldown`: Frame counter for cooldown (runtime only)

**GAMESTATE** additions:
- `is_in_tall_grass`: Boolean flag for vision restriction
- `sticky_floor_counter`: Counts movement attempts on sticky floor
- `last_teleport_frame`: Tracks last teleport for cooldown

### Key Constants

```c
#define TELEPORT_PAD_COOLDOWN_FRAMES (2 * FRAMES_PER_SECOND)  // 2 seconds
#define STICKY_FLOOR_SLOWDOWN 2                                // Half speed
#define TALL_GRASS_VISION_RANGE 2                              // 2 tiles visible
```

### Rendering Order

1. Passive obstacles (floor layer)
2. Totems and power-ups
3. Player
4. Enemies
5. Static obstacles
6. Level boundaries
7. Aggressive obstacles

Tall grass vision restriction is applied in `SafePrintEntity()` before rendering any entity.

## Usage Tips

### Teleport Pads
- Link pads in pairs for predictable behavior
- Place pads strategically to create shortcuts or puzzles
- Can be single tile `(x,y) (x,y)` or small areas
- Remember the 2-second cooldown when designing levels

### Sticky Floor
- Use to create challenging navigation sections
- Combine with enemies to increase difficulty
- Works well near obstacles to test player timing
- Player needs to plan movement carefully

### Tall Grass
- Create stealth/exploration sections
- Hide enemies or totems in grass
- Test player memory and spatial awareness
- Combine with other obstacles for complex challenges

## Testing

Test levels provided:
- `levels/simple_passive.txt` - Simple examples of each obstacle type
- `levels/passive_test.txt` - Comprehensive test with all features

To test:
```bash
make
./warrior_race
# Select "New Game"
# Choose "simple_passive" or "passive_test" level
```

## Future Enhancements

Possible improvements:
- Variable teleport cooldown per pad
- Directional sticky floors (harder to move in certain directions)
- Adjustable tall grass vision range per area
- Ice floors (sliding effect)
- Moving platforms (conveyor belts)
