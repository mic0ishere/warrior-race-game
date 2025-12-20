[**progress log**](progress_log.md)

[**project description**](project_description.pdf)

### useful resources

- [cpp reference (with c stdlib)](https://cplusplus.com/reference/)
- [event-driven programming (timers, ncurses)](https://www.cs.hunter.cuny.edu/~sweiss/course_materials/unix_lecture_notes/chapter_06.pdf)
- [ncurses programming howto](https://invisible-island.net/ncurses/howto/NCURSES-Programming-HOWTO.html)
- [sample ncurses program](sample_bird_flight.c)
- [function length calculator](https://gre-v-el.github.io/Cpp-Func-Length-Counter/)

### building and running the game

```bash
make run        # Build and run the game in one step

make            # Build the game
make clean      # Remove build artifacts
make rebuild    # Clean and rebuild

./warrior_race  # Run the game
```

### power-ups

The game now includes collectible power-ups that enhance gameplay:

- **Shield (S)** - Cyan - Negates the next enemy collision (not environmental hazards)
- **Double Points (D)** - Yellow - Doubles all points earned for 10 seconds
- **Extra Life (L)** - Green - Instantly grants one additional life
- **Time Freeze (F)** - Blue - Freezes all enemies for 5 seconds

Active power-ups are displayed in the status bar with their remaining duration.

### level format

Power-ups can be added to level files in the `[POWERUPS]` section:

```
[POWERUPS]
SHIELD (5,5)
DOUBLE (20,20)
LIFE (35,15)
FREEZE (10,25)
```
