#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEBUG

WINDOW *InitScreen(void) {
  WINDOW *win = initscr();

  if (win == NULL) {
    fprintf(stderr, "Error initialising ncurses.\n");
    exit(EXIT_FAILURE);
  }

  start_color();
  init_pair(PLAY_COLOR, COLOR_BLACK, COLOR_WHITE);
  init_pair(STAT_COLOR, COLOR_YELLOW + 8, COLOR_BLUE);
  init_pair(DEBUG_COLOR, COLOR_RED + 8, COLOR_BLACK);
  init_pair(END_WIN_COLOR, COLOR_WHITE, COLOR_GREEN);
  init_pair(END_LOSE_COLOR, COLOR_WHITE, COLOR_RED);
  init_pair(ENEMY_COLOR, COLOR_WHITE + 8, COLOR_RED);
  init_pair(TOTEM_COLOR, COLOR_BLACK, COLOR_YELLOW + 8);
  init_pair(OBSTACLE_COLOR, COLOR_BLACK, COLOR_WHITE);
  init_pair(PLAYER_COLOR, COLOR_BLACK, COLOR_WHITE);

  noecho();
  curs_set(0);

  return win;
}

void CleanWin(WIN *W, int bo) {
  int i, j;

  wattron(W->window, COLOR_PAIR(W->color));

  if (bo)
    box(W->window, 0, 0);

  for (i = bo; i < W->rows - bo; i++)
    for (j = bo; j < W->cols - bo; j++)
      mvwprintw(W->window, i, j, "%s", " ");

  wrefresh(W->window);
}

WIN *InitWin(WINDOW *parent, int rows, int cols, int y, int x, int color,
             int bo, int is_debug) {

  WIN *W = (WIN *)malloc(sizeof(WIN));
  
  if (W == NULL) {
    return NULL;
  }

  W->x = x;
  W->y = y;
  W->rows = rows;
  W->cols = cols;
  W->color = color;

  W->window = subwin(parent, rows, cols, y, x);
  
  if (W->window == NULL) {
    free(W);
    return NULL;
  }

#ifndef DEBUG
  if (is_debug) {
    return W;
  }
#else
  (void)is_debug;
#endif

  CleanWin(W, bo);
  nodelay(W->window, TRUE);
  wrefresh(W->window);

  return W;
}

int InitAllWindows(WINDOW *screenwin, WINDOWS *windows) {
  windows->playwin = InitWin(screenwin, ROWS, COLS, OFFY, OFFX, 
                             PLAY_COLOR, BORDER, FALSE);
  if (windows->playwin == NULL) {
    return 1;
  }

  windows->statwin = InitWin(screenwin, STAT_ROWS, COLS, ROWS + OFFY, OFFX,
                             STAT_COLOR, BORDER, FALSE);
  if (windows->statwin == NULL) {
    delwin(windows->playwin->window);
    free(windows->playwin);
    return 2;
  }

  windows->debugwin = InitWin(screenwin, 3, COLS, ROWS + OFFY + STAT_ROWS + 1,
                              OFFX, DEBUG_COLOR, BORDER, TRUE);
  if (windows->debugwin == NULL) {
    delwin(windows->playwin->window);
    delwin(windows->statwin->window);
    free(windows->playwin);
    free(windows->statwin);
    return 3;
  }

  return 0;
}

void CleanupWindows(WINDOW *screenwin, WINDOWS *windows) {
  if (windows->playwin) {
    delwin(windows->playwin->window);
    free(windows->playwin);
  }
  if (windows->statwin) {
    delwin(windows->statwin->window);
    free(windows->statwin);
  }
  if (windows->debugwin) {
    delwin(windows->debugwin->window);
    free(windows->debugwin);
  }
  if (screenwin) {
    delwin(screenwin);
  }
  endwin();
  refresh();
}

void EndGame(WIN *W, char *msg, int color_pair) {
  W->color = color_pair;
  CleanWin(W, BORDER);

  mvwprintw(W->window, W->rows / 2, (W->cols - strlen(msg)) / 2, "%s", msg);

  wrefresh(W->window);
}

void ShowStatus(WIN *W, GAMESTATE *state) {
  CleanWin(W, BORDER);

  mvwprintw(W->window, 1, 2, "Score: %d/%d  Time left: %d s  Lives: %d/%d",
            state->score, state->level->win_score,
            state->level->time_limit - state->time_elapsed, state->lives_left,
            state->level->lives);

  char pos_str[32];
  snprintf(pos_str, sizeof(pos_str), "(%d,%d)", state->player_pos.x,
           state->player_pos.y);
  mvwprintw(W->window, 1, COLS - BORDER - strlen(pos_str), "%s", pos_str);

  mvwprintw(W->window, 2, 2, "%s", "[WASD]=Move [Q]=Quit");

  wrefresh(W->window);
}

void ShowDebug(WIN *W, const char *msg) {
#ifdef DEBUG
  wattron(W->window, COLOR_PAIR(W->color));

  box(W->window, 0, 0);

  for (int i = strlen(msg) + 2; i < W->cols - 2; i++)
    mvwprintw(W->window, 1, i, "%s", " ");

  mvwprintw(W->window, 1, 2, "%s", msg);

  wrefresh(W->window);
#else
  (void)W;
  (void)msg;
#endif
}

void LevelToScreenCoords(WIN *W, int level_x, int level_y, GAMESTATE *state,
                         int *screen_x, int *screen_y) {
  int center_x = W->cols / 2;
  int center_y = W->rows / 2;

  *screen_x = center_x + (level_x - state->player_pos.x);
  *screen_y = center_y + (state->player_pos.y - level_y);
}

void SafePrintEntity(WIN *W, int screen_x, int screen_y, const char *symbol,
                     GAMESTATE *state, int color_pair) {
  if (screen_x < -1 || screen_x > state->level->width || screen_y < -1 ||
      screen_y > state->level->height) {
    return;
  }

  POSITION draw_pos;
  LevelToScreenCoords(W, screen_x, screen_y, state, &draw_pos.x, &draw_pos.y);

  if (draw_pos.x >= 0 && draw_pos.x < W->cols && 
      draw_pos.y >= 0 && draw_pos.y < W->rows) {
    wattron(W->window, COLOR_PAIR(color_pair));
    mvwprintw(W->window, draw_pos.y, draw_pos.x, "%s", symbol);
  }
}

void RenderPlayArea(WIN *W, GAMESTATE *state) {
  CleanWin(W, 0);

  for (int i = 0; i < state->level->num_totems; i++) {
    TOTEM *totem = &state->level->totems[i];
    if (totem->is_collected)
      continue;
    SafePrintEntity(W, totem->pos.x, totem->pos.y, "T", state, TOTEM_COLOR);
  }

  for (int i = 0; i < state->level->num_enemies; i++) {
    ENEMY *enemy = &state->level->enemies[i];
    SafePrintEntity(W, enemy->current_pos.x, enemy->current_pos.y, "E", state,
                    ENEMY_COLOR);
  }

  for (int i = 0; i < state->level->num_obstacles; i++) {
    OBSTACLE *obs = &state->level->obstacles[i];

    for (int x = obs->pos1.x; x <= obs->pos2.x; x++) {
      for (int y = obs->pos1.y; y <= obs->pos2.y; y++) {
        SafePrintEntity(W, x, y, "#", state, OBSTACLE_COLOR);
      }
    }
  }

  for (int x = 0; x < state->level->width; x++) {
    SafePrintEntity(W, x, -1, "-", state, OBSTACLE_COLOR);
    SafePrintEntity(W, x, state->level->height, "-", state, OBSTACLE_COLOR);
  }

  for (int y = 0; y < state->level->height; y++) {
    SafePrintEntity(W, -1, y, "|", state, OBSTACLE_COLOR);
    SafePrintEntity(W, state->level->width, y, "|", state, OBSTACLE_COLOR);
  }

  SafePrintEntity(W, state->player_pos.x, state->player_pos.y, "@", state,
                  PLAYER_COLOR);

  wrefresh(W->window);
}
