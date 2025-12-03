# Warrior Race Game

[**project description**](project_description.pdf)

## Project Structure

The code is organized into multiple modules for better maintainability:

- **types.h** - Type definitions, constants, and data structures
- **window.h/window.c** - Window management, display, and rendering functions
- **level.h/level.c** - Level loading and configuration
- **game.h/game.c** - Game state, collision detection, and gameplay logic
- **main.c** - Main entry point and game loop

## Building

Use the provided Makefile:

```bash
make          # Build the game
make clean    # Remove build artifacts
make rebuild  # Clean and rebuild
```

Or compile manually:

```bash
gcc -Wall -Wextra -Wpedantic -Wformat-security -o warrior_race main.c window.c level.c game.c -lncurses
```

## Running

```bash
./warrior_race
```

## Useful Resources

- [cpp reference (with c stdlib)](https://cplusplus.com/reference/)
- [event-driven programming (timers, ncurses)](https://www.cs.hunter.cuny.edu/~sweiss/course_materials/unix_lecture_notes/chapter_06.pdf)
- [ncurses programming howto](https://invisible-island.net/ncurses/howto/NCURSES-Programming-HOWTO.html)
- [sample ncurses program](sample_bird_flight.c)
