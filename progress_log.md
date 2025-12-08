# Phase 1

### 25-11-2025

- Created basic game structure with ncurses windows for gameplay, status, and debugging.

### 27-11-2025

- Proper game ending with final status display
- Implemented player movement in a game area and input handling.
- Reading level data from a file.
- Added collision detection and basic game mechanics (totem collection, enemy movement).
- Scoring system based on totems collected.

# Phase 2

### 03-12-2025

- Used Github Copilot (Agent mode) to help with security vulnerabilities, memory leaks and code optimization.
  - It required instructions as to where is the main file is located, what is the project outline and at which stage the project is.
  - Reverted the decision to suppress unused parameter warnings, and instead chose to refactor the code to eliminate unused parameters.
- Collaborated with it using a pull request workflow.
  - Manually asked to refactor few functions and type definitions in order to improve readability.
  - Asked it to refactor the game into modular architecture, which it did successfully.
- Improved code quality, readability, and maintainability.
- Learned how to properly work with pull requests on Github.

### 08-12-2025
- Asked ChatGPT on ideas to expand the game.
  - It provided multiple ideas, some realistic gameplay changes (player power-ups, more enemy types, dynamic obstacles), possible technical improvements (ranking system, entity animations)
  - Others were more complex or not possible and would require significant time to implement (procedural generation, inventory, audio effects)
  - After few prompts for refining, a roadmap of features to implement was created