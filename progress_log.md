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

### 14-12-2025

- Add a "proper" level, with a designed layout and multiple enemy types.
  - Done it manually, as I wanted to have full control over the design and gameplay experience.

### 15-12-2025

- Implemented a ranking system to store high scores in a file.
  - Manually created a formula to calculate final scores based on totems collected, time taken, and lives remaining.
  - Used Copilot Agent (Claude Sonnet 4.5) to help with file handling and displaying rankings.
  - After it completed initial implementation, I have provided additional instructions to make code quality and style in line with the rest of the project.
  - AI did a good job, in the meantime I have updated screen timings and simplified ranking file saving.
  - While testing, thought that saving rankings in a binary file is less user-friendly, so I have asked Copilot to refactor the code to use a text file instead, which it did successfully.

- IDEA: Create more detailed AI prompts with step-by-step instruction to minimize the need for manual adjustments. Use other AI tools that specialize in text generation to help craft better prompts for coding AIs like Copilot. 
  1. Initial task description <-> ChatGPT for prompt refinement
  2. Copilot Agent for code generation <-> Iterative improvements
  3. Manual review and refactoring

### 17-12-2025

- Updated `copilot_instructions.md` (file which is used by Copilot Agent to understand the project) to reflect new development requirements.
  - Revised the project description, technical stack, and current development phase.
  - Added instructions about code style preferences, quality and maintainability expectations.
  - This will hopefully help Copilot provide more relevant suggestions in future development sessions.

### 20-12-2025

- Added power-ups: shield, double points, extra life, and time freeze.
  - Firstly, asked ChatGPT for ideas on power-ups and prompt refinement for Copilot Agent (Claude Sonnet 4.5).
  - While initial implementation Copilot suggested a good structure for power-up management, there were some bugs with power-up user interface display and their effects.
  - After prompting Copilot to fix the issues, it has provided working solutions, although they still lacked some critical user experience elements.
  - I have once again asked Copilot to address these problems, which it did successfully.
  - Finally, after testing and playing the game, I have manually adjusted timings to ensure balanced gameplay.
  - Moreover, while reviewing the code, I extracted LoadPowerup out of the LoadLevel function to shorten it.

- Added passive obstacles: teleport pads, sticky floor, and tall grass.
  - Once again, I have first collaborated with ChatGPT to create a prompt for Copilot Agent (Claude Sonnet 4.5).
  - The initial code lacked some features, but with a prompt with list of required changes, Copilot was able to deliver working implementations.
  - As usual, I conducted a review of the generated code and refactored some parts that were not in line with my code style preferences.

### 21-12-2025

- Improved legacy levels to include new enemies, obstacles, and power-ups.
  - Copilot Agent (GPT-5.1-Codex-Max) assisted with level file updates and simulating gameplay.
  - It helped to update old level files to include new entities and obstacles.
  - While the generated levels were functional, they weren't very engaging. 
  - IDEA: This is an area where I would need to provide significalny more detailed gameplay mechanics and level design instructions in future, to get better results from AI. However, as I'm not a good level designer myself, creation of such prompts is out of my current capabilities.

- Added an instructions screen in the menu to explain game controls and mechanics.
  - Here, I have firstly used Google Gemini to generate the prompt for Copilot Agent (Claude Sonnet 4.5).
  - Gemini's output was more structured and detailed, which helped Copilot to create a better implementation.
  - Copilot's implementation was extremely good, having only small issues in regards to correct entity character.
  - I have asked Copilot to fix these problems and extract the instructions text into a separate file for easier future modifications, which it did successfully.
  - The code generated was successful, but I wasn't happy with the solution used to read the instructions file, so I decided to ask Copilot to refactor it into a separate C header file, which it did well.
  - Moreover, I asked Copilot to extract and improve the text rendering function for instructions screen, which it did successfully.
  - Tired of prompting Copilot for minor fixes, I have manually updated line rendering function to make it more versatile and reusable.

- Performed a thorough code review of the entire project.
 - Asked Copilot Agent (Claude Sonnet 4.5) to identify bugs, quality issues and general areas for improvement.
 - It provided a comprehensive list of suggestions, including refactoring opportunities, performance optimizations, and code style improvements.
 - I have manually reviewed these suggestions and implemented the most relevant ones with the help of Copilot Agent and Copilot Next Edits.
 - Additionally, while modifying the code, I have fixed some minor bugs and inconsistencies that I found during testing.