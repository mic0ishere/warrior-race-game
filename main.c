#include "game.h"
#include "level.h"
#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
  WINDOW *screenwin = InitScreen();
  WINDOWS windows = {NULL, NULL, NULL};

  int result = InitAllWindows(screenwin, &windows);
  if (result != 0) {
    endwin();
    const char *window_names[] = {"", "play window", "status window", "debug window"};
    fprintf(stderr, "Error: Failed to create %s\n", window_names[result]);
    return EXIT_FAILURE;
  }

  GAMESTATE *state = InitGame();

  MainLoop(windows.playwin, windows.statwin, windows.debugwin, state);

  switch (state->status) {
  case GAME_TIMEOUT:
    EndGame(windows.playwin, "Time's up!", END_LOSE_COLOR);
    break;
  case OUT_OF_LIVES:
    EndGame(windows.playwin, "Out of lives!", END_LOSE_COLOR);
    break;
  case USER_QUIT:
    EndGame(windows.playwin, "You quit the game.", END_LOSE_COLOR);
    break;
  case CONTINUE:
    EndGame(windows.playwin, "Game ended unexpectedly.", END_LOSE_COLOR);
    break;
  case WIN_POINTS:
    EndGame(windows.playwin, "You collected enough points!", END_WIN_COLOR);
    break;
  }

  char end_msg[COLS];
  snprintf(end_msg, sizeof(end_msg), "Your final score: %d", state->score);
  mvwprintw(windows.playwin->window, windows.playwin->rows / 2 + 1,
            (windows.playwin->cols - strlen(end_msg)) / 2, "%s", end_msg);
  wrefresh(windows.playwin->window);

  CleanWin(windows.statwin, BORDER);
  sleep(2);

  FreeLevel(state->level);
  free(state);
  CleanupWindows(screenwin, &windows);

  return 0;
}
