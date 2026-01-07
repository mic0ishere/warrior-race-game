#include "game.h"
#include "level.h"
#include "menu.h"
#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Main entry point - initializes ncurses and runs the main game menu loop
int main(void) {
  // Initialize ncurses screen with colors and settings
  WINDOW *screenwin = InitScreen();

  // Main menu loop - continues until user exits
  while (1) {
    // Create temporary window for menu display
    WIN *mainwin = InitWin(screenwin, ROWS + STAT_ROWS, COLS, OFFY, OFFX,
                           DEFAULT_COLOR, BORDER, 0);

    // Display main menu and get user's choice
    int menu_choice = ShowMainMenu(mainwin);

    // Display main menu and get user's choice
    int menu_choice = ShowMainMenu(mainwin);

    // Handle exit choice - cleanup and break from main loop
    if (menu_choice == MENU_EXIT) {
      CleanupWin(mainwin);
      break;
    } else if (menu_choice == MENU_INSTRUCTIONS) {
      // Show game instructions and return to menu
      ShowInstructions(mainwin);
      continue;
    } else if (menu_choice == MENU_RANKING) {
      // Display ranking table and return to menu
      ShowRanking(mainwin);
      continue;
    } else if (menu_choice == MENU_NEW_GAME) {
      // Show level selector and load chosen level
      LEVEL *loaded_level = ShowLevelSelector(mainwin);

      if (!loaded_level) {
        continue; // User cancelled, go back to main menu
      }

      CleanupWin(mainwin);

      WINDOWS windows = {NULL, NULL, NULL};

      // Initialize all three game windows (play, status, debug)
      int result = InitAllWindows(screenwin, &windows);
      if (result != 0) {
        // Display error if window creation fails
        const char *window_names[] = {"play window", "status window",
                                      "debug window"};
        fprintf(stderr, "Error: Failed to create %s\n",
                window_names[result - 1]);
        FreeLevel(loaded_level);
        break;
      }

      // Initialize game state with loaded level
      GAMESTATE *state = InitGame(loaded_level);

      // Run main game loop (20 FPS)
      MainLoop(windows.playwin, windows.statwin, windows.debugwin, state);

      // Display end game screen with score calculation
      FinishGame(windows.playwin, state);

      CleanWin(windows.statwin, BORDER);
      sleep(state->status == USER_QUIT ? 1 : 5); // Shorter delay if user quit

      // Cleanup game resources
      FreeLevel(state->level);
      free(state);
      CleanupWindows(&windows);
    }
  }

  // Cleanup ncurses and restore terminal
  delwin(screenwin);
  endwin();
  refresh();

  return 0;
}
