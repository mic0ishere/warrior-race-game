#include "window.h"
#include "game.h"
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
  init_pair(DEFAULT_COLOR, COLOR_WHITE, COLOR_BLACK);
  init_pair(PLAY_COLOR, COLOR_BLACK, COLOR_WHITE);
  init_pair(STAT_COLOR, COLOR_YELLOW + 8, COLOR_BLUE);
  init_pair(DEBUG_COLOR, COLOR_RED + 8, COLOR_BLACK);
  init_pair(END_WIN_COLOR, COLOR_WHITE, COLOR_GREEN);
  init_pair(END_LOSE_COLOR, COLOR_WHITE, COLOR_RED);
  init_pair(ENEMY_COLOR, COLOR_WHITE + 8, COLOR_RED);
  init_pair(TOTEM_COLOR, COLOR_BLACK, COLOR_YELLOW + 8);
  init_pair(OBSTACLE_COLOR, COLOR_BLACK, COLOR_WHITE);
  init_pair(PLAYER_COLOR, COLOR_BLACK, COLOR_WHITE);
  init_pair(SPIKE_COLOR, COLOR_WHITE + 8, COLOR_MAGENTA);
  init_pair(FIREJET_COLOR, COLOR_WHITE + 8, COLOR_RED + 8);
  init_pair(TELEPORTER_WARNING_COLOR, COLOR_WHITE + 8, COLOR_MAGENTA + 8);
  init_pair(CHASER_SLEEPING_COLOR, COLOR_WHITE, COLOR_BLUE);
  init_pair(CHASER_ANGRY_COLOR, COLOR_WHITE + 8, COLOR_RED + 8);
  init_pair(POWERUP_SHIELD_COLOR, COLOR_BLACK, COLOR_CYAN + 8);
  init_pair(POWERUP_DOUBLE_COLOR, COLOR_BLACK, COLOR_YELLOW);
  init_pair(POWERUP_LIFE_COLOR, COLOR_BLACK, COLOR_GREEN + 8);
  init_pair(POWERUP_FREEZE_COLOR, COLOR_BLACK, COLOR_BLUE + 8);
  init_pair(TELEPORT_PAD_COLOR, COLOR_BLACK, COLOR_MAGENTA);
  init_pair(STICKY_FLOOR_COLOR, COLOR_BLACK, COLOR_YELLOW);
  init_pair(TALL_GRASS_COLOR, COLOR_BLACK, COLOR_GREEN);

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
  windows->playwin =
      InitWin(screenwin, ROWS, COLS, OFFY, OFFX, PLAY_COLOR, BORDER, FALSE);
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

void CleanupWindows(WINDOWS *windows) {
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
}

void CleanupWin(WIN *W) {
  if (W) {
    wclear(W->window);
    wrefresh(W->window);
    delwin(W->window);
    free(W);
  }
}

void EndGame(WIN *W, char *msg, int color_pair) {
  W->color = color_pair;
  CleanWin(W, BORDER);

  mvwprintw(W->window, W->rows / 2, (W->cols - strlen(msg)) / 2, "%s", msg);

  wrefresh(W->window);
}

void ShowStatus(WIN *W, GAMESTATE *state) {
  CleanWin(W, BORDER);

  mvwprintw(W->window, 1, 2, "Score: %d/%d  Time left: %d s  Lifes: %d/%d",
            state->score, state->level->win_score,
            state->level->time_limit - state->time_elapsed, state->lifes_left,
            state->level->lifes);

  char pos_str[32];
  snprintf(pos_str, sizeof(pos_str), "(%d,%d)", state->player_pos.x,
           state->player_pos.y);
  mvwprintw(W->window, 1, COLS - BORDER - strlen(pos_str), "%s", pos_str);

  mvwprintw(W->window, 2, 2, "%s", "[WASD]=Move [Q]=Quit [P]=Pause");

  char powerup_status[COLS];
  int status_pos = 0;

  if (state->has_shield) {
    status_pos +=
        snprintf(powerup_status + status_pos, COLS - status_pos, "[Shield] ");
  }

  if (state->extra_life_timer > 0) {
    status_pos +=
        snprintf(powerup_status + status_pos, COLS - status_pos, "[+1 Life] ");
  }

  for (int i = 0; i < state->num_active_powerups; i++) {
    if (state->active_powerups[i].is_active) {
      int seconds = state->active_powerups[i].frames_remaining / FRAMES_PER_SECOND;
      switch (state->active_powerups[i].type) {
      case POWERUP_DOUBLE_POINTS:
        status_pos += snprintf(powerup_status + status_pos, COLS - status_pos,
                               "[2x Points:%ds] ", seconds);
        break;
      case POWERUP_TIME_FREEZE:
        status_pos += snprintf(powerup_status + status_pos, COLS - status_pos,
                               "[Freeze:%ds] ", seconds);
        break;
      default:
        break;
      }
    }
  }

  if (status_pos > 0) {
    mvwprintw(W->window, 3, 2, "%s", powerup_status);
  }

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

void ShowLifeLoss(WIN *W, LIFE_LOST_STATUS life_loss) {
  char *msg = "You lost a life!";
  char *reason_msg = NULL;

  switch (life_loss) {
  case LIFE_PATROL_KILL:
    reason_msg = "Patrollers have caught you!";
    break;
  case LIFE_CHASER_KILL:
    reason_msg = "You can't escape the chaser!";
    break;
  case LIFE_TELEPORTER_KILL:
    reason_msg = "An enemy has teleported onto you!";
    break;
  case LIFE_FIRE_JET:
    reason_msg = "Burned by fire jet!";
    break;
  case LIFE_SPIKE_WALK:
    reason_msg = "Impaled on spikes!";
    break;
  default:
    reason_msg = "An unknown force has defeated you!";
    break;
  }

  wattron(W->window, COLOR_PAIR(END_LOSE_COLOR));
  mvwprintw(W->window, W->rows / 2 + 2, (W->cols - strlen(msg)) / 2, "%s", msg);
  mvwprintw(W->window, W->rows / 2 + 3, (W->cols - strlen(reason_msg)) / 2,
            "%s", reason_msg);
  wattron(W->window, COLOR_PAIR(W->color));
  wrefresh(W->window);
}

void ShowAlert(WIN *W, const char *msg) {
  wattron(W->window, COLOR_PAIR(END_LOSE_COLOR));
  mvwprintw(W->window, W->rows / 2 + 2, (W->cols - strlen(msg)) / 2, "%s", msg);
  wattron(W->window, COLOR_PAIR(W->color));
  wrefresh(W->window);
}

void LevelToScreenCoords(WIN *W, int level_x, int level_y, GAMESTATE *state,
                         int *screen_x, int *screen_y) {
  int center_x = W->cols / 2;
  int center_y = W->rows / 2;

  *screen_x = center_x + (level_x - state->player_pos.x);
  *screen_y = center_y + (state->player_pos.y - level_y);
}

void SafePrintEntity(WIN *W, int level_x, int level_y, const char *symbol,
                     GAMESTATE *state, int color_pair) {
  if (level_x < -1 || level_x > state->level->width || level_y < -1 ||
      level_y > state->level->height) {
    return;
  }
  
  // Apply tall grass vision restriction
  if (state->is_in_tall_grass) {
    int dx = abs(level_x - state->player_pos.x);
    int dy = abs(level_y - state->player_pos.y);
    
    if (dx > TALL_GRASS_VISION_RANGE || dy > TALL_GRASS_VISION_RANGE) {
      return;
    }
  }

  POSITION screen_pos;
  LevelToScreenCoords(W, level_x, level_y, state, &screen_pos.x, &screen_pos.y);

  if (screen_pos.x >= 0 && screen_pos.x < W->cols && screen_pos.y >= 0 &&
      screen_pos.y < W->rows) {
    wattron(W->window, COLOR_PAIR(color_pair));
    mvwprintw(W->window, screen_pos.y, screen_pos.x, "%s", symbol);
  }
}

void RenderPlayArea(WIN *W, GAMESTATE *state) {
  CleanWin(W, 0);

  // Render passive obstacles first (as floor tiles)
  for (int i = 0; i < state->level->num_passive_obstacles; i++) {
    PASSIVE_OBSTACLE *obs = &state->level->passive_obstacles[i];
    
    const char *symbol = "?";
    int color = DEFAULT_COLOR;
    
    switch (obs->type) {
    case PASSIVE_TELEPORT_PAD:
      symbol = "T";
      color = TELEPORT_PAD_COLOR;
      break;
    case PASSIVE_STICKY_FLOOR:
      symbol = "~";
      color = STICKY_FLOOR_COLOR;
      break;
    case PASSIVE_TALL_GRASS:
      symbol = ",";
      color = TALL_GRASS_COLOR;
      break;
    default:
      continue;
    }
    
    for (int x = obs->pos1.x; x <= obs->pos2.x; x++) {
      for (int y = obs->pos1.y; y <= obs->pos2.y; y++) {
        SafePrintEntity(W, x, y, symbol, state, color);
      }
    }
  }

  for (int i = 0; i < state->level->num_totems; i++) {
    TOTEM *totem = &state->level->totems[i];
    if (totem->is_collected)
      continue;
    SafePrintEntity(W, totem->pos.x, totem->pos.y, "T", state, TOTEM_COLOR);
  }

  for (int i = 0; i < state->level->num_powerups; i++) {
    POWERUP *powerup = &state->level->powerups[i];
    if (powerup->is_collected)
      continue;

    const char *symbol = "?";
    int color = DEFAULT_COLOR;

    switch (powerup->type) {
    case POWERUP_SHIELD:
      symbol = "S";
      color = POWERUP_SHIELD_COLOR;
      break;
    case POWERUP_DOUBLE_POINTS:
      symbol = "D";
      color = POWERUP_DOUBLE_COLOR;
      break;
    case POWERUP_EXTRA_LIFE:
      symbol = "L";
      color = POWERUP_LIFE_COLOR;
      break;
    case POWERUP_TIME_FREEZE:
      symbol = "F";
      color = POWERUP_FREEZE_COLOR;
      break;
    default:
      continue;
    }

    SafePrintEntity(W, powerup->pos.x, powerup->pos.y, symbol, state, color);
  }

  // Render player
  SafePrintEntity(W, state->player_pos.x, state->player_pos.y, "@", state,
                  PLAYER_COLOR);

  for (int i = 0; i < state->level->num_enemies; i++) {
    ENEMY *enemy = &state->level->enemies[i];
    int enemy_color = ENEMY_COLOR;

    // Determine color based on enemy type and state
    if (enemy->type == ENEMY_TELEPORTER) {
      // Flash warning color when about to teleport
      int frames_since_last_teleport =
          state->frames_elapsed - enemy->last_teleport_frame;
      int frames_until_teleport = (TELEPORTER_INTERVAL_FRAMES / enemy->speed) -
                                  frames_since_last_teleport;

      if (frames_until_teleport >= 0 &&
          frames_until_teleport <= TELEPORTER_WARNING_FRAMES) {
        // Alternate between warning color and normal enemy color (3:2 ratio)
        enemy_color = frames_until_teleport % 5 >= 2 ? TELEPORTER_WARNING_COLOR
                                                     : ENEMY_COLOR;
      }
    } else if (enemy->type == ENEMY_CHASER) {
      // Change color based on chase state
      enemy_color =
          enemy->is_chasing ? CHASER_ANGRY_COLOR : CHASER_SLEEPING_COLOR;
    }

    SafePrintEntity(W, enemy->current_pos.x, enemy->current_pos.y, "E", state,
                    enemy_color);
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

  for (int i = 0; i < state->level->num_aggressive_obstacles; i++) {
    AGGRESSIVE_OBSTACLE *obs = &state->level->aggressive_obstacles[i];

    if (obs->type == AGGRESSIVE_SPIKE) {
      const char *spike_symbols[] = {"^", "v", "<", ">"};

      if (obs->is_active) {
        SafePrintEntity(W, obs->pos.x, obs->pos.y,
                        spike_symbols[obs->direction % 4], state, SPIKE_COLOR);
      }
    } else if (obs->type == AGGRESSIVE_FIREJET) {
      SafePrintEntity(W, obs->pos.x, obs->pos.y, "F", state, FIREJET_COLOR);

      if (obs->is_active) {
        for (int j = 1; j <= obs->fire_length; j++) {
          POSITION fire_pos = obs->pos;
          MoveInDirection(&fire_pos, obs->direction, j);
          SafePrintEntity(W, fire_pos.x, fire_pos.y, "*", state, FIREJET_COLOR);
        }
      }
    }
  }

  wrefresh(W->window);
}
