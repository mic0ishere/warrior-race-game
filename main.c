#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEBUG

#define QUIT 'q'
#define UP 'w'
#define DOWN 's'
#define LEFT 'a'
#define RIGHT 'd'
#define NOKEY ERR

// Timing and speed
#define FRAMES_PER_SECOND 20
#define LEVEL_FILE "level1.txt"

#define PLAY_COLOR 1     // Main play window color
#define STAT_COLOR 2     // Status window color
#define DEBUG_COLOR 3    // Debug window color
#define END_WIN_COLOR 4  // End game win color
#define END_LOSE_COLOR 5 // End game lose color
#define ENEMY_COLOR 6
#define TOTEM_COLOR 7
#define OBSTACLE_COLOR 8
#define PLAYER_COLOR 9

// Window dimensions and position
#define BORDER 1    // Border width (in characters)
#define COLS 80     // Play window width (columns)
#define ROWS 20     // Play window height (rows)
#define STAT_ROWS 5 // Status window height (rows)
#define OFFY 2      // Y offset from top of screen
#define OFFX 5      // X offset from left of screen

// Game status codes
#define CONTINUE 0
#define GAME_TIMEOUT 1
#define OUT_OF_LIVES 2
#define USER_QUIT 3
#define WIN_POINTS 10

#define TRUE 1
#define FALSE 0
#define DIR_FORWARD 1
#define DIR_BACKWARD -1
#define MAX_DEBUG_LEN COLS - 3
#define MAX_LINE_LEN 256
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

const int FRAME_TIME_MS = 1000 / FRAMES_PER_SECOND;

typedef struct {
  WINDOW *window;
  int x, y;
  int rows, cols;
  int color;
} WIN;

typedef struct {
  int x, y;
} POSITION;

typedef struct {
  POSITION pos;
  int points;
  int is_collected;
} TOTEM;

typedef struct {
  POSITION pos1;
  POSITION pos2;
} OBSTACLE;

typedef enum {
  ENEMY_PATROL_HORIZONTAL,
  ENEMY_PATROL_VERTICAL,
  ENEMY_TYPE_UNKNOWN
} ENEMY_TYPE;

typedef struct {
  POSITION pos1;
  POSITION pos2;
  POSITION current_pos;
  ENEMY_TYPE type;
  int direction; // 1=forward, -1=backward
  int speed;
} ENEMY;

typedef struct {
  int time_limit;
  int lives;
  int win_score;
  int width, height;
  POSITION start_pos;

  int num_totems;
  TOTEM *totems;

  int num_obstacles;
  OBSTACLE *obstacles;

  int num_enemies;
  ENEMY *enemies;
} LEVEL;

typedef struct {
  int score;
  int time_elapsed;
  int frames_elapsed;
  int lives_left;
  char debug_msg[MAX_DEBUG_LEN];
  int status;
  LEVEL *level;
  POSITION player_pos;
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
  init_pair(END_WIN_COLOR, COLOR_WHITE, COLOR_GREEN);
  init_pair(END_LOSE_COLOR, COLOR_WHITE, COLOR_RED);
  init_pair(ENEMY_COLOR, COLOR_WHITE + 8, COLOR_RED);
  init_pair(TOTEM_COLOR, COLOR_BLACK, COLOR_YELLOW + 8);
  init_pair(OBSTACLE_COLOR, COLOR_BLACK, COLOR_WHITE);
  init_pair(PLAYER_COLOR, COLOR_BLACK, COLOR_WHITE);

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
             int bo, int is_debug) {

  WIN *W = (WIN *)malloc(sizeof(WIN));
  
  if (W == NULL) {
    return NULL;
  }

  // Store window properties
  W->x = x;
  W->y = y;
  W->rows = rows;
  W->cols = cols;
  W->color = color;

  // Create ncurses subwindow
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
  (void)is_debug; // Suppress unused parameter warning when DEBUG is defined
#endif

  // Clear the window
  CleanWin(W, bo);

  // Set input mode: delay==0 means non-blocking (for real-time games)
  nodelay(W->window, TRUE);

  // Display the window
  wrefresh(W->window);

  return W;
}

// Generic function to dynamically add entities to level arrays
// Used for totems, obstacles, and enemies during level loading
void *AddLevelEntity(void *array, size_t entry_size, int *num_entries,
                     const void *new_entry) {
  void *resized_array = realloc(array, (*num_entries + 1) * entry_size);

  if (resized_array == NULL) {
    free(array);
    return NULL;
  }

  array = resized_array;
  memcpy((char *)array + (*num_entries) * entry_size, new_entry, entry_size);
  (*num_entries)++;

  return array;
}

void FreeLevel(LEVEL *level) {
  if (level != NULL) {
    free(level->totems);
    free(level->obstacles);
    free(level->enemies);
    free(level);
  }
}

ENEMY LoadEnemy(const char *line, int enemy_id) {
  ENEMY enemy;
  sscanf(line, "(%d,%d) (%d,%d) %d %d", &enemy.pos1.x, &enemy.pos1.y,
         &enemy.pos2.x, &enemy.pos2.y, &enemy.speed, &enemy.direction);

  if (enemy.direction != DIR_BACKWARD && enemy.direction != DIR_FORWARD) {
    enemy.direction = DIR_FORWARD;
    fprintf(stderr,
            "Warning: Enemies[%d] has invalid direction. Set to FORWARD.\n",
            enemy_id);
  }

  int verified_speed = MIN(MAX(enemy.speed, 1), 5);
  if (verified_speed != enemy.speed) {
    fprintf(stderr,
            "Warning: Enemies[%d] speed %d out of bounds. Clamped to %d.\n",
            enemy_id, enemy.speed, verified_speed);
    enemy.speed = verified_speed;
  }

  enemy.current_pos = enemy.direction == DIR_BACKWARD ? enemy.pos2 : enemy.pos1;

  if (enemy.pos1.y == enemy.pos2.y) {
    enemy.type = ENEMY_PATROL_HORIZONTAL;
  } else if (enemy.pos1.x == enemy.pos2.x) {
    enemy.type = ENEMY_PATROL_VERTICAL;
  } else {
    enemy.type = ENEMY_TYPE_UNKNOWN;
    fprintf(stderr, "Warning: Enemies[%d] with unknown patrol type.\n",
            enemy_id);
  }

  return enemy;
}

LEVEL *LoadLevel(const char *filename) {
  LEVEL *level = (LEVEL *)malloc(sizeof(LEVEL));
  
  if (level == NULL) {
    return NULL;
  }

  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    free(level);
    return NULL;
  }

  level->totems = NULL;
  level->obstacles = NULL;
  level->enemies = NULL;
  level->num_totems = 0;
  level->num_obstacles = 0;
  level->num_enemies = 0;

  char line[MAX_LINE_LEN];
  char *current_section = NULL;

  while (fgets(line, MAX_LINE_LEN, file)) {
    line[strcspn(line, "\n")] = 0; // Remove trailing newline
    if (strlen(line) == 0 || line[0] == '#') {
      continue; // Skip empty lines and comments
    }

    if (line[0] == '[' && line[strlen(line) - 1] == ']') {
      free(current_section); // Free previous section name
      current_section = strdup(line); // Duplicate section name and store it
      if (current_section == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for section name\n");
        FreeLevel(level);
        fclose(file);
        return NULL;
      }
      continue;
    }

    if (current_section != NULL && strcmp(current_section, "[CONFIG]") == 0) {
      sscanf(line, "TIME=%d", &level->time_limit);
      sscanf(line, "LIVES=%d", &level->lives);
      sscanf(line, "WIDTH=%d", &level->width);
      sscanf(line, "HEIGHT=%d", &level->height);
      sscanf(line, "START_X=%d", &level->start_pos.x);
      sscanf(line, "START_Y=%d", &level->start_pos.y);
      sscanf(line, "WIN_SCORE=%d", &level->win_score);
    } else if (current_section != NULL && strcmp(current_section, "[TOTEMS]") == 0) {
      TOTEM new_totem;
      new_totem.is_collected = FALSE;
      sscanf(line, "(%d,%d) %d", &new_totem.pos.x, &new_totem.pos.y,
             &new_totem.points);

      level->totems = (TOTEM *)AddLevelEntity(level->totems, sizeof(TOTEM),
                                              &level->num_totems, &new_totem);
      if (level->totems == NULL) {
        free(current_section);
        FreeLevel(level);
        fclose(file);
        return NULL;
      }

    } else if (current_section != NULL && strcmp(current_section, "[OBSTACLES]") == 0) {
      OBSTACLE new_obstacle;
      sscanf(line, "(%d,%d) (%d,%d)", &new_obstacle.pos1.x,
             &new_obstacle.pos1.y, &new_obstacle.pos2.x, &new_obstacle.pos2.y);

      level->obstacles =
          (OBSTACLE *)AddLevelEntity(level->obstacles, sizeof(OBSTACLE),
                                     &level->num_obstacles, &new_obstacle);
      if (level->obstacles == NULL) {
        free(current_section);
        FreeLevel(level);
        fclose(file);
        return NULL;
      }
    } else if (current_section != NULL && strcmp(current_section, "[ENEMIES]") == 0) {
      ENEMY enemy = LoadEnemy(line, level->num_enemies);
      level->enemies = (ENEMY *)AddLevelEntity(level->enemies, sizeof(ENEMY),
                                               &level->num_enemies, &enemy);
      if (level->enemies == NULL) {
        free(current_section);
        FreeLevel(level);
        fclose(file);
        return NULL;
      }
    }
  }

  free(current_section);
  fclose(file);
  return level;
}

GAMESTATE *InitGame() {
  GAMESTATE *state = (GAMESTATE *)malloc(sizeof(GAMESTATE));

  if (state == NULL) {
    fprintf(stderr, "Error allocating memory for game state\n");
    exit(EXIT_FAILURE);
  }

  state->level = LoadLevel(LEVEL_FILE);

  if (state->level == NULL) {
    free(state);
    fprintf(stderr, "Error loading level file: %s\n", LEVEL_FILE);
    exit(EXIT_FAILURE);
  }

  state->score = 0;
  state->time_elapsed = 0;
  state->frames_elapsed = 0;
  state->lives_left = state->level->lives;
  state->status = CONTINUE;
  state->player_pos = state->level->start_pos;

  snprintf(state->debug_msg, MAX_DEBUG_LEN, "Game initialized.");

  return state;
}

void EndGame(WIN *W, char *msg, int color_pair) {
  W->color = color_pair;
  CleanWin(W, BORDER);

  // Display centered goodbye message
  mvwprintw(W->window, W->rows / 2, (W->cols - strlen(msg)) / 2, "%s", msg);

  wrefresh(W->window);
}

void ShowStatus(WIN *W, GAMESTATE *state) {
  CleanWin(W, BORDER);

  // Display game statistics
  mvwprintw(W->window, 1, 2, "Score: %d/%d  Time left: %d s  Lives: %d/%d",
            state->score, state->level->win_score,
            state->level->time_limit - state->time_elapsed, state->lives_left,
            state->level->lives);

  char pos_str[32];
  snprintf(pos_str, sizeof(pos_str), "(%d,%d)", state->player_pos.x,
           state->player_pos.y);
  mvwprintw(W->window, 1, COLS - BORDER - strlen(pos_str), pos_str,
            state->player_pos.x, state->player_pos.y);

  // Display controls
  mvwprintw(W->window, 2, 2, "[WASD]=Move [Q]=Quit");

  // Update display
  wrefresh(W->window);
}

void ShowDebug(WIN *W, const char *msg) {
#ifdef DEBUG
  // Set status bar color
  wattron(W->window, COLOR_PAIR(W->color));

  box(W->window, 0, 0);

  // Display debug message
  for (int i = strlen(msg) + 2; i < W->cols - 2; i++)
    mvwprintw(W->window, 1, i, " ");

  mvwprintw(W->window, 1, 2, "%s", msg);

  // Update display
  wrefresh(W->window);
#endif
}

// Convert level coordinates to screen coordinates with camera centered on player
// This implements scrolling by keeping the player at the center of the screen
void LevelToScreenCoords(WIN *W, int level_x, int level_y, GAMESTATE *state,
                         int *screen_x, int *screen_y) {
  int center_x = W->cols / 2;
  int center_y = W->rows / 2;

  *screen_x = center_x + (level_x - state->player_pos.x);
  // This may seem counter-intuitive, but in ncurses Y increases downwards
  // (inverse to a Cartesian plane)
  *screen_y = center_y + (state->player_pos.y - level_y);
}

int CheckWithinBounds(int pos1_x, int pos1_y, int pos2_x, int pos2_y,
                      POSITION point) {
  return (point.x >= pos1_x && point.x <= pos2_x && point.y >= pos1_y &&
          point.y <= pos2_y);
}

int CheckCollision(GAMESTATE *state, POSITION wanted_pos) {
  // Check obstacles
  for (int i = 0; i < state->level->num_obstacles; i++) {
    OBSTACLE *obs = &state->level->obstacles[i];
    if (CheckWithinBounds(obs->pos1.x, obs->pos1.y, obs->pos2.x, obs->pos2.y,
                          wanted_pos)) {
      return FALSE;
    }
  }

  // Check if inside the game area
  return CheckWithinBounds(0, 0, state->level->width - 1,
                           state->level->height - 1, wanted_pos);
}

void SafePrintEntity(WIN *W, int screen_x, int screen_y, const char *symbol,
                     GAMESTATE *state, int color_pair) {
  // Don't draw outside level boundaries
  if (screen_x < -1 || screen_x > state->level->width || screen_y < -1 ||
      screen_y > state->level->height) {
    return;
  }

  POSITION draw_pos;
  LevelToScreenCoords(W, screen_x, screen_y, state, &draw_pos.x, &draw_pos.y);

  if (CheckWithinBounds(0, 0, W->cols - 1, W->rows - 1, draw_pos)) {
    wattron(W->window, COLOR_PAIR(color_pair));
    mvwprintw(W->window, draw_pos.y, draw_pos.x, "%s", symbol);
  }
}

void CollectTotems(GAMESTATE *state) {
  for (int i = 0; i < state->level->num_totems; i++) {
    TOTEM *totem = &state->level->totems[i];
    if (!totem->is_collected && totem->pos.x == state->player_pos.x &&
        totem->pos.y == state->player_pos.y) {
      state->score += totem->points;
      totem->is_collected = TRUE;
    }
  }
}

POSITION NextEnemyPosition(ENEMY *enemy) {
  POSITION next_pos = enemy->current_pos;
  int move_amount = enemy->direction;

  if (enemy->type == ENEMY_PATROL_HORIZONTAL) {
    next_pos.x += move_amount;
  } else if (enemy->type == ENEMY_PATROL_VERTICAL) {
    next_pos.y += move_amount;
  }

  return next_pos;
}

// Update enemy positions based on their patrol patterns and speed
void MoveEnemies(GAMESTATE *state) {
  for (int i = 0; i < state->level->num_enemies; i++) {
    ENEMY *enemy = &state->level->enemies[i];
    if (enemy->type == ENEMY_TYPE_UNKNOWN) {
      continue; // Skip enemies with unknown type
    }

    // Control enemy movement speed: higher speed = more frequent moves
    if (state->frames_elapsed % (FRAMES_PER_SECOND / enemy->speed) != 0) {
      continue; // Move based on enemy speed
    }

    POSITION wanted_pos = NextEnemyPosition(enemy);

    if (!CheckCollision(state, wanted_pos) ||
        !CheckWithinBounds(enemy->pos1.x, enemy->pos1.y, enemy->pos2.x,
                           enemy->pos2.y, wanted_pos)) {
      enemy->direction *= -1; // Reverse direction
      enemy->current_pos = NextEnemyPosition(enemy);
      continue;
    }

    enemy->current_pos = wanted_pos;
  }
}

void CheckPlayerEnemyCollision(GAMESTATE *state) {
  for (int i = 0; i < state->level->num_enemies; i++) {
    ENEMY *enemy = &state->level->enemies[i];
    if (enemy->current_pos.x == state->player_pos.x &&
        enemy->current_pos.y == state->player_pos.y) {
      state->lives_left--;
      state->player_pos = state->level->start_pos; // Reset player position
      break;
    }
  }
}

void RenderPlayArea(WIN *W, GAMESTATE *state) {
  CleanWin(W, 0);

  // Draw totems
  for (int i = 0; i < state->level->num_totems; i++) {
    TOTEM *totem = &state->level->totems[i];
    if (totem->is_collected)
      continue;
    SafePrintEntity(W, totem->pos.x, totem->pos.y, "T", state, TOTEM_COLOR);
  }

  // Draw enemies
  for (int i = 0; i < state->level->num_enemies; i++) {
    ENEMY *enemy = &state->level->enemies[i];
    SafePrintEntity(W, enemy->current_pos.x, enemy->current_pos.y, "E", state,
                    ENEMY_COLOR);
  }

  // Draw obstacles
  for (int i = 0; i < state->level->num_obstacles; i++) {
    OBSTACLE *obs = &state->level->obstacles[i];

    for (int x = obs->pos1.x; x <= obs->pos2.x; x++) {
      for (int y = obs->pos1.y; y <= obs->pos2.y; y++) {
        SafePrintEntity(W, x, y, "#", state, OBSTACLE_COLOR);
      }
    }
  }

  // Draw level boundaries
  for (int x = 0; x < state->level->width; x++) {
    SafePrintEntity(W, x, -1, "-", state, OBSTACLE_COLOR);
    SafePrintEntity(W, x, state->level->height, "-", state, OBSTACLE_COLOR);
  }

  for (int y = 0; y < state->level->height; y++) {
    SafePrintEntity(W, -1, y, "|", state, OBSTACLE_COLOR);
    SafePrintEntity(W, state->level->width, y, "|", state, OBSTACLE_COLOR);
  }

  // Draw player
  SafePrintEntity(W, state->player_pos.x, state->player_pos.y, "@", state,
                  PLAYER_COLOR);

  wrefresh(W->window);
}

void MainLoop(WIN *playwin, WIN *statwin, WIN *debugwin, GAMESTATE *state) {
  int ch;
  POSITION wanted_pos;

  while (1) {
    ch = wgetch(statwin->window);

    if (ch == QUIT) {
      state->status = USER_QUIT;
      break;
    }

    wanted_pos.x = state->player_pos.x;
    wanted_pos.y = state->player_pos.y;

    if (ch == UP) {
      wanted_pos.y++;
    } else if (ch == DOWN) {
      wanted_pos.y--;
    } else if (ch == LEFT) {
      wanted_pos.x--;
    } else if (ch == RIGHT) {
      wanted_pos.x++;
    }

    if (CheckCollision(state, wanted_pos)) {
      state->player_pos = wanted_pos;
    }

    snprintf(state->debug_msg, MAX_DEBUG_LEN, "Player pos: (%d,%d)",
             state->player_pos.x, state->player_pos.y);

    RenderPlayArea(playwin, state);
    CollectTotems(state);
    MoveEnemies(state);
    CheckPlayerEnemyCollision(state);

    state->frames_elapsed++;
    state->time_elapsed = state->frames_elapsed / FRAMES_PER_SECOND;

    ShowDebug(debugwin, state->debug_msg);
    ShowStatus(statwin, state);
    wrefresh(playwin->window); // Refresh play window

    if (state->time_elapsed >= state->level->time_limit) {
      state->status = GAME_TIMEOUT;
      break;
    }

    if (state->lives_left <= 0) {
      state->status = OUT_OF_LIVES;
      break;
    }

    if (state->score >= state->level->win_score) {
      state->status = WIN_POINTS;
      break;
    }

    flushinp(); // Clear input buffer to avoid key press accumulation

    usleep(FRAME_TIME_MS * 1000); // Sleep to control frame rate
  }
}

int main(void) {
  // Initialize ncurses and color system
  WINDOW *screenwin = InitScreen();

  // Play window: 20x80 cells, positioned at (2,5) with border
  WIN *playwin =
      InitWin(screenwin, ROWS, COLS, OFFY, OFFX, PLAY_COLOR, BORDER, FALSE);
  
  if (playwin == NULL) {
    endwin();
    fprintf(stderr, "Error: Failed to create play window\n");
    return EXIT_FAILURE;
  }

  // Status window: 4x80 cells, positioned below play window
  WIN *statwin = InitWin(screenwin, STAT_ROWS, COLS, ROWS + OFFY, OFFX,
                         STAT_COLOR, BORDER, FALSE);
  
  if (statwin == NULL) {
    delwin(playwin->window);
    free(playwin);
    endwin();
    fprintf(stderr, "Error: Failed to create status window\n");
    return EXIT_FAILURE;
  }

  // Debug window: 3x80 cells, positioned below status window
  WIN *debugwin = InitWin(screenwin, 3, COLS, ROWS + OFFY + STAT_ROWS + 1, OFFX,
                          DEBUG_COLOR, BORDER, TRUE);
  
  if (debugwin == NULL) {
    delwin(playwin->window);
    delwin(statwin->window);
    free(playwin);
    free(statwin);
    endwin();
    fprintf(stderr, "Error: Failed to create debug window\n");
    return EXIT_FAILURE;
  }

  GAMESTATE *state = InitGame();

  MainLoop(playwin, statwin, debugwin, state);

  switch (state->status) {
  case GAME_TIMEOUT:
    EndGame(playwin, "Time's up!", END_LOSE_COLOR);
    break;

  case OUT_OF_LIVES:
    EndGame(playwin, "Out of lives!", END_LOSE_COLOR);
    break;

  case USER_QUIT:
    EndGame(playwin, "You quit the game.", END_LOSE_COLOR);
    break;

  case CONTINUE:
    EndGame(playwin, "Game ended unexpectedly.", END_LOSE_COLOR);
    break;

  case WIN_POINTS:
    EndGame(playwin, "You collected enough points!", END_WIN_COLOR);
    break;
  }

  char end_msg[COLS];
  snprintf(end_msg, sizeof(end_msg), "Your final score: %d", state->score);
  mvwprintw(playwin->window, playwin->rows / 2 + 1,
            (playwin->cols - strlen(end_msg)) / 2, "%s", end_msg);
  wrefresh(playwin->window);

  // EndGame(statwin, "Statistics will be saved to a file", STAT_COLOR);
  CleanWin(statwin, BORDER);
  sleep(2); // Wait 2 seconds before closing

  // Delete ncurses windows
  delwin(playwin->window);
  delwin(statwin->window);
  delwin(debugwin->window);
  delwin(screenwin);

  // Close ncurses
  endwin();
  refresh();

  // Free allocated memory
  FreeLevel(state->level);
  free(state);
  free(playwin);
  free(statwin);
  free(debugwin);

  return 0;
}