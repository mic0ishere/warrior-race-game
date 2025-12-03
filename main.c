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

  FinishGame(windows.playwin, state);

  CleanWin(windows.statwin, BORDER);
  sleep(2);

  FreeLevel(state->level);
  free(state);
  CleanupWindows(screenwin, &windows);

  return 0;
}
