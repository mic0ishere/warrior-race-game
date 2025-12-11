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
    // Show main menu
    int menu_choice = ShowMainMenu(screenwin);

    if (menu_choice == MENU_EXIT) {
      break;
    } else if (menu_choice == MENU_RANKING) {
      ShowRanking(screenwin);
      continue;
    } else if (menu_choice == MENU_NEW_GAME) {
      // Show level selector
      LEVEL_SELECT_RESULT level_result = ShowLevelSelector(screenwin);

      if (!level_result.selected) {
        continue; // User cancelled, go back to main menu
      }

      // Start game with selected level
      WINDOWS windows = {NULL, NULL, NULL};

      int result = InitAllWindows(screenwin, &windows);
      if (result != 0) {
        const char *window_names[] = {"", "play window", "status window",
                                      "debug window"};
        wclear(screenwin);
        mvwprintw(screenwin, ROWS / 2, (COLS - 40) / 2,
                  "Error: Failed to create %s", window_names[result]);
        wrefresh(screenwin);
        sleep(2);
        continue;
      }

      GAMESTATE *state = InitGame(level_result.filename);

      MainLoop(windows.playwin, windows.statwin, windows.debugwin, state);

      FinishGame(windows.playwin, state);

      CleanWin(windows.statwin, BORDER);
      sleep(2);

      FreeLevel(state->level);
      free(state);
      CleanupWindows(screenwin, &windows);

      // Re-initialize screen for menu
      screenwin = InitScreen();
    }
  }

  endwin();
  return 0;
}
