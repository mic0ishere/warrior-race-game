#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define QUIT 'q'
#define UP 'w'
#define DOWN 's'
#define LEFT 'a'
#define RIGHT 'd'
#define NOKEY ERR

#define FRAMES_PER_SECOND 20
#define GAME_LENGTH 120 // Game length in seconds

#define PLAY_COLOR 1  // Main window color
#define STAT_COLOR 2  // Status window color
#define DEBUG_COLOR 3 // Play area color

// Window dimensions and position
#define BORDER 1    // Border width (in characters)
#define COLS 80     // Play window width (columns)
#define ROWS 20     // Play window height (rows)
#define STAT_ROWS 5 // Status window height (rows)
#define OFFY 2      // Y offset from top of screen
#define OFFX 5      // X offset from left of screen

#define TRUE 1
#define FALSE 0
#define MAX_DEBUG_LEN COLS - 3

const int FRAME_TIME_MS = 1000 / FRAMES_PER_SECOND;

typedef struct {
  WINDOW *window;
  int x, y;
  int rows, cols;
  int color;
} WIN;

typedef struct {
  int score;
  int time_elapsed;
  int frames_elapsed;
  int lives;
  char debug_msg[MAX_DEBUG_LEN];
} GAMESTATE;

WINDOW *InitScreen() {
  WINDOW *win = initscr();

  if (win == NULL) {
    fprintf(stderr, "Error initialising ncurses.\n");
    exit(EXIT_FAILURE);
  }

  // https://www.linuxjournal.com/content/about-ncurses-colors-0
  start_color();
  init_pair(PLAY_COLOR, COLOR_BLACK, COLOR_WHITE);
  init_pair(STAT_COLOR, COLOR_YELLOW + 8, COLOR_BLUE);

  init_pair(DEBUG_COLOR, COLOR_RED + 8, COLOR_BLACK);

  // Don't echo typed characters to screen
  noecho();

  // Make cursor invisible (curs_set: 0=invisible, 1=normal, 2=very visible)
  curs_set(0);

  return win;
}

void CleanWin(WIN *W, int bo) {
  int i, j;

  // Set window color
  wattron(W->window, COLOR_PAIR(W->color));

  // Draw border if requested
  if (bo)
    box(W->window, 0, 0);

  // Fill window with spaces (clearing it)
  for (i = bo; i < W->rows - bo; i++)
    for (j = bo; j < W->cols - bo; j++)
      mvwprintw(W->window, i, j, " ");

  // Refresh to show changes
  wrefresh(W->window);
}

WIN *InitWin(WINDOW *parent, int rows, int cols, int y, int x, int color,
             int bo) {
  WIN *W = (WIN *)malloc(sizeof(WIN));

  // Store window properties
  W->x = x;
  W->y = y;
  W->rows = rows;
  W->cols = cols;
  W->color = color;

  // Create ncurses subwindow
  W->window = subwin(parent, rows, cols, y, x);

  // Clear the window
  CleanWin(W, bo);

  // Set input mode: delay==0 means non-blocking (for real-time games)
  nodelay(W->window, TRUE);

  // Display the window
  wrefresh(W->window);

  return W;
}

GAMESTATE *InitGame() {
  GAMESTATE *state = (GAMESTATE *)malloc(sizeof(GAMESTATE));

  state->score = 0;
  state->time_elapsed = 0;
  state->frames_elapsed = 0;
  state->lives = 3;

  snprintf(state->debug_msg, MAX_DEBUG_LEN, "Game initialized.");

  return state;
}

void EndGame(WIN *W, char *msg, int sleep_seconds) {
  // Clear the window
  CleanWin(W, 1);

  // Display centered goodbye message
  mvwprintw(W->window, W->rows / 2, (W->cols - strlen(msg)) / 2, msg);
  wrefresh(W->window);

  // Wait 2 seconds before closing
  sleep(sleep_seconds);
}

void ShowStatus(WIN *W, GAMESTATE *state) {
  // Set status bar color
  wattron(W->window, COLOR_PAIR(W->color));

  // Draw border around status bar
  box(W->window, 0, 0);

  // Display game statistics
  mvwprintw(W->window, 1, 2, "Score: %d  Time: %d s  Lives: %d", state->score,
            state->time_elapsed, state->lives);

  // Display controls
  mvwprintw(W->window, 2, 2, "[WASD]=Move [Q]=Quit");

  // Update display
  wrefresh(W->window);
}

void ShowDebug(WIN *W, const char *msg) {
  // Set status bar color
  wattron(W->window, COLOR_PAIR(W->color));

  box(W->window, 0, 0);

  // Display debug message
  for (int i = strlen(msg) + 2; i < W->cols - 2; i++)
    mvwprintw(W->window, 1, i, " ");

  mvwprintw(W->window, 1, 2, "%s", msg);

  // Update display
  wrefresh(W->window);
}

void MainLoop(WIN *playwin, WIN *statwin, WIN *debugwin, GAMESTATE *state) {
  int ch;

  while (1) {
    ch = wgetch(statwin->window);

    if (ch == QUIT)
      break;

    // todo: update game state etc

    state->frames_elapsed++;
    state->time_elapsed = state->frames_elapsed / FRAMES_PER_SECOND;

    ShowDebug(debugwin, state->debug_msg);
    ShowStatus(statwin, state);

    wrefresh(playwin->window); // Refresh play window

    flushinp(); // Clear input buffer to avoid key press accumulation

    usleep(FRAME_TIME_MS * 1000); // Sleep to control frame rate
  }
}

int main(int argc, char *argv[]) {
  // Initialize ncurses and color system
  WINDOW *screenwin = InitScreen();

  // Play window: 20x80 cells, positioned at (2,5) with border
  WIN *playwin = InitWin(screenwin, ROWS, COLS, OFFY, OFFX, PLAY_COLOR, BORDER);

  // Status window: 4x80 cells, positioned below play window
  WIN *statwin = InitWin(screenwin, STAT_ROWS, COLS, ROWS + OFFY, OFFX,
                         STAT_COLOR, BORDER);

  // Debug window: 3x80 cells, positioned below status window
  WIN *debugwin = InitWin(screenwin, 3, COLS, ROWS + OFFY + STAT_ROWS + 1, OFFX,
                          DEBUG_COLOR, BORDER);

  GAMESTATE *state = InitGame();

  MainLoop(playwin, statwin, debugwin, state);

  EndGame(statwin, "Statistics will be saved to a file", 0);
  EndGame(playwin, "Game Over! Thanks for playing!", 2);

  // Delete ncurses windows
  delwin(playwin->window);
  delwin(statwin->window);
  delwin(debugwin->window);
  delwin(screenwin);

  // Close ncurses
  endwin();
  refresh();

  // Free allocated memory
  free(state);
  free(playwin);
  free(statwin);
  free(debugwin);

  return 0;
}