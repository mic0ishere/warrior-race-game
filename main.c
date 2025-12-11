#include "game.h"
#include "level.h"
#include "menu.h"
#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
  WINDOW *screenwin = InitScreen();

  while (1) {
    WIN *mainwin = InitWin(screenwin, ROWS + STAT_ROWS, COLS, OFFY, OFFX,
                           DEFAULT_COLOR, BORDER, 0);

    // Show main menu
    int menu_choice = ShowMainMenu(mainwin);

    if (menu_choice == MENU_EXIT) {
      CleanupWin(mainwin);
      break;
    } else if (menu_choice == MENU_RANKING) {
      ShowRanking(mainwin);
      continue;
    } else if (menu_choice == MENU_NEW_GAME) {
      // Load level from level selector
      LEVEL *loaded_level = ShowLevelSelector(mainwin);

      if (!loaded_level) {
        continue; // User cancelled, go back to main menu
      }

      CleanupWin(mainwin);

      WINDOWS windows = {NULL, NULL, NULL};

      int result = InitAllWindows(screenwin, &windows);
      if (result != 0) {
        const char *window_names[] = {"play window", "status window",
                                      "debug window"};
        fprintf(stderr, "Error: Failed to create %s\n",
                window_names[result - 1]);
        FreeLevel(loaded_level);
        break;
      }

      GAMESTATE *state = InitGame(loaded_level);

      MainLoop(windows.playwin, windows.statwin, windows.debugwin, state);

      FinishGame(windows.playwin, state);

      CleanWin(windows.statwin, BORDER);
      sleep(2);

      FreeLevel(state->level);
      free(state);
      CleanupWindows(&windows);
    }
  }

  delwin(screenwin);
  endwin();
  refresh();

  return 0;
}
