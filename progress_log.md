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

### 10-12-2025

- Using the roadmap created with ChatGPT, started implementing new gameplay features.

- Implemented new enemy types (random walkers, chasers, teleporters) and aggressive obstacles (spikes and fire jets) by using Github Copilot agent (Claude Sonnet 4.5)
  - It excelled at generating the enemy movement logic based on the descriptions provided - the game was working flawlessly after the first implementation.
  - Some of the code required minor adjustments for quality purposes, mostly related to legacy config parameters and integration with existing code
  - I have manually refactored some of the code to improve readability and maintainability

- Added a proper life loss handling with an alert message.
  - Copilot inline suggestions helped with implementation, providing nice messages
  - Suggested delay mechanism was too short and needed to be adjusted manually

- Implemented pause functionality in the game, without AI assistance.

- IDEA: After heavy AI-assisted development, a proper code review would be beneficial to ensure code quality and maintainability. I would suggest asking Copilot to identify potential issues before a thorough manual review.

### 11-12-2025

- Implemented menu system with new game, view ranking, and exit options.
  - Used Copilot (Claude Sonnet 4.5) to help with game menu addition.
  - There were some issues with the initial implementation, so I had to rework parts of it manually.
  - The implementation of new window management wasn't in line with the existing code style and structure.
  - Manually updated the menu layout and structure for better user experience.
  - Refined the code for better readability and maintainability.

- Asked ChatGPT for ideas on function naming - it suggested more descriptive names which I adopted for better code readability.

- Created level selector to choose from multiple levels at the start of the game.
  - Copilot assisted with scrolling logic and level loading.
  - The "design" it suggested wasn't looking good, so I reimplemented the layout manually for better aesthetics.
  - Again, I refactored some of the code for clarity.

### 12-12-2025

- Enhanced enemy behavior:
  - For teleporters, added a flashing indicator before teleporting.
  - For chasers, implemented a sleeping state, where they display in a different color when the player is outside of their vision.
  - Copilot provided good initial implementations, but some adjustments were needed for timing and visual effects.
  - While testing, I have identified and fixed existing bugs in the entity collission and movement logic (for teleporters the speed variable had no effect, player could walk into the source of firejet obstacle and teleporters could teleport onto player's starting position).