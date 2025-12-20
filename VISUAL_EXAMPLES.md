# Passive Obstacles Visual Examples

## Example 1: Teleport Pad Pair

```
Level Layout (simple_passive.txt excerpt):

    0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20
 0  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--
 1  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
 2  |  | @|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
 3  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
 4  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
 5  |  |  |  |  |  | E|  |  |  |  |  |  |  |  |  |  |  |  |  | P|  <- Pad 0
 6  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
 7  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
 8  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
 9  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
10  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--
11  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
12  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
13  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
14  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
15  |  |  |  |  | P|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  <- Pad 1
16  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |

Legend:
  @ = Player
  P = Teleport Pad (magenta)
  E = Enemy
  
Behavior:
  - Player at (2,2)
  - Walking to Pad 0 at (20,5) → Teleports to Pad 1 at (5,15)
  - Walking to Pad 1 at (5,15) → Teleports to Pad 0 at (20,5)
  - 2 second cooldown between teleports
```

## Example 2: Sticky Floor

```
Level Layout:

    8  9 10 11 12 13 14 15
 4  |  |  |  |  |  |  |  |
 5  |  |  |~~|~~|~~|~~|  |  <- Sticky Floor Area
 6  |  |  |~~|~~|~~|~~|  |
 7  |  |  |~~|~~|~~|~~|  |
 8  |  |  |~~|~~|~~|~~|  |
 9  |  |  |  |  |  |  |  |

Legend:
  ~ = Sticky Floor (yellow)

Movement sequence:
  Normal terrain:  Press W → Move up 1 tile
  Sticky floor:    Press W → No movement
                   Press W → Move up 1 tile
                   Press W → No movement
                   Press W → Move up 1 tile

Effect: Requires 2 key presses per tile movement
```

## Example 3: Tall Grass Vision

```
Without Tall Grass (normal vision):

   10 11 12 13 14 15 16 17 18 19 20
10 |  |  |  |  |  | T|  |  | E|  |
11 |  |  |  |  |  |  |  |  |  |  |
12 |  |  |  |  |  |  |  |  |  |  |
13 |  |  |  | E|  |  |  |  |  |  |
14 |  |  |  |  |  |  |  |  |  | T|
15 |  |  |  |  |  | @|  |  |  |  |
16 |  |  |  |  |  |  |  |  |  |  |

Player can see all entities (Totems, Enemies, etc.)


With Tall Grass (player inside grass at center):

   10 11 12 13 14 15 16 17 18 19 20
10 |  |  |  |  |  |  |  |  |  |  |
11 |  |  |  |  |  |  |  |  |  |  |
12 |  |  |  |  |  |  |  |  |  |  |
13 |  |  |  | E|  |  |  |  |  |  |  <- Visible (within 2 tiles)
14 |  |  |  |  |,,|,,|,,|  |  |  |
15 |  |  |  |  |,,| @|,,|  |  |  |  <- Player in grass
16 |  |  |  |  |,,|,,|,,|  |  |  |
17 |  |  |  |  |  |  |  |  |  |  |
18 |  |  |  |  |  |  |  |  |  |  |

Legend:
  , = Tall Grass (green)
  @ = Player
  E = Enemy
  T = Totem

Vision radius: 2 tiles in all directions from player
- Entities beyond 2 tiles are not rendered
- Creates fog-of-war exploration effect
```

## Combined Example: All Three Obstacles

```
Challenging level section:

    0  1  2  3  4  5  6  7  8  9 10 11 12 13 14
 0  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--
 1  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
 2  |  | @|  |  |  |  |  |  |  | T|  |  |  |  |
 3  |  |  |  |  |~~|~~|~~|  |  |  |  |  |  |  |  <- Sticky floor
 4  |  |  |  |  |~~|~~|~~|  |  |  |  |  |  |  |
 5  |  |  |  |  |~~|~~|~~|  |  |  |  |  |  |  |
 6  |  |  |  |  |  |  |  |  |,,|,,|,,|,,|  |  |  <- Tall grass
 7  |  |  |  |  |  |  |  |  |,,|,,|,,|,,|  |  |
 8  |  |  | P|  |  |  |  |  |,,|,,|,,|,,|  |  |  <- Teleport pad 0
 9  |  |  |  |  |  |  |  |  |,,|,,|,,|,,|  |  |
10  |  |  |  |  |  |  |  |  |  |  |  |  | P|  |  <- Teleport pad 1

Gameplay challenge:
1. Player starts at (2,2)
2. Must navigate through sticky floor (slower movement)
3. Collect totem at (10,2)
4. Enter tall grass (limited vision)
5. Use teleport pad to quickly navigate
6. Strategic use of obstacles creates puzzle-like gameplay
```

## Implementation Notes

### Rendering Order (bottom to top):
1. Background (clear)
2. **Passive obstacles (P, ~, ,)** ← Rendered first as floor
3. Totems (T)
4. Power-ups (S, D, L, F)
5. Player (@)
6. Enemies (E)
7. Static obstacles (#)
8. Level boundaries (-, |)
9. Aggressive obstacles (^, v, <, >, F, *)

### Key Features:
- **Teleport**: 40 frames (2 sec) cooldown, validates destination
- **Sticky Floor**: Counter-based, 2 key presses = 1 movement
- **Tall Grass**: Modifies SafePrintEntity(), 2-tile visibility radius
