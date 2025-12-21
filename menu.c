#include "menu.h"
#include "instructions.h"
#include "level.h"
#include "ranking.h"
#include "window.h"
#include <dirent.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Main menu items
static const char *menu_items[MENU_ITEMS] = {"New Game", "Read Instructions",
                                             "View Ranking", "Exit"};

// Instructions string
static const char *instructions =
    "Use [W/S] to navigate, [space] to select, [Q] to exit";

// Comparison function for qsort
static int compare_strings(const void *a, const void *b) {
  return strcmp((const char *)a, (const char *)b);
}

int ShowMainMenu(WIN *W) {
  WINDOW *win = W->window;
  int highlight = 0;
  int choice = -1;
  int c;

  // Wait for user input
  nodelay(win, FALSE);

  while (1) {
    wclear(win);
    box(win, 0, 0);

    // Title
    const char *title = "=== WARRIOR RACE ===";
    const char *subtitle = "Terminal Roguelike Adventure";
    mvwprintw(win, 2, (W->cols - strlen(title)) / 2, "%s", title);
    mvwprintw(win, 3, (W->cols - strlen(subtitle)) / 2, "%s", subtitle);

    // Menu items
    for (int i = 0; i < MENU_ITEMS; i++) {
      if (i == highlight)
        wattron(win, A_REVERSE);

      mvwprintw(win, 8 + i * 2, (W->cols - 20) / 2, "%d. %s", i + 1,
                menu_items[i]);

      if (i == highlight)
        wattroff(win, A_REVERSE);
    }

    // Display instructions
    mvwprintw(win, W->rows - 2, (W->cols - strlen(instructions)) / 2, "%s",
              instructions);

    wrefresh(win);

    c = wgetch(win);

    switch (c) {
    case UP:
      highlight = (highlight - 1 + MENU_ITEMS) % MENU_ITEMS;
      break;
    case DOWN:
      highlight = (highlight + 1) % MENU_ITEMS;
      break;
    case CONFIRM:
      choice = highlight;
      return choice;
    case QUIT:
      return MENU_EXIT;
    default:
      if (c >= '1' && c <= '0' + MENU_ITEMS) {
        return c - '1';
      }

      break;
    }
  }

  return choice;
}

int ScanLevels(char levels[][256]) {
  DIR *dir;
  struct dirent *entry;
  int count = 0;

  dir = opendir("levels");
  if (dir == NULL) {
    return 0;
  }

  while ((entry = readdir(dir)) != NULL && count < MAX_LEVELS) {
    size_t len = strlen(entry->d_name);
    if (len > 4 && len < 248 && strcmp(entry->d_name + len - 4, ".txt") == 0) {
      snprintf(levels[count], 256, "levels/%s", entry->d_name);
      count++;
    }
  }

  closedir(dir);
  qsort(levels, count, 256, compare_strings);
  return count;
}

char *GetLevelName(const char *filename) {
  const char *basename = strrchr(filename, '/'); // Find last '/'
  if (basename) {
    basename++; // Skip the '/'
  } else {
    basename = filename;
  }

  // Safe string copy, leaving room for null terminator
  // Note: Returns pointer to static buffer - does not need to be freed
  static char level_name[256];
  strncpy(level_name, basename, sizeof(level_name) - 1);
  level_name[sizeof(level_name) - 1] = '\0';

  // Remove .txt extension
  char *ext = strstr(level_name, ".txt");
  if (ext) {
    *ext = '\0';
  }

  return level_name;
}

static void RenderLevelList(WIN *W, char levels[][256], int num_levels,
                            int highlight, int scroll_offset) {
  int start_y = 5;
  int visible_items = W->rows - 10;
  const int column_start = (W->cols) / 2 - 16;

  for (int i = scroll_offset;
       i < num_levels && i <= scroll_offset + visible_items; i++) {
    if (i == highlight) {
      wattron(W->window, A_REVERSE);
    }

    char *level_name = GetLevelName(levels[i]);
    mvwprintw(W->window, start_y + (i - scroll_offset), column_start, "%d. %s",
              i + 1, level_name);

    if (i == highlight) {
      wattroff(W->window, A_REVERSE);
    }
  }

  if (scroll_offset + visible_items < num_levels - 1) {
    mvwhline(W->window, start_y + visible_items, 1, ' ', W->cols - 2);
    mvwprintw(W->window, start_y + visible_items, column_start,
              "... %d more levels\t\t\t",
              num_levels - (scroll_offset + visible_items));
  }
}

static int HandleLevelInput(int c, int *highlight, int num_levels) {
  switch (c) {
  case UP:
    *highlight = (*highlight - 1 + num_levels) % num_levels;
    return 0;
  case DOWN:
    *highlight = (*highlight + 1) % num_levels;
    return 0;
  case CONFIRM:
    return 1;
  case QUIT:
    return -1;
  default:
    if (c >= '1' && c <= '0' + num_levels) {
      *highlight = c - '1';
      return 1;
    }
    return 0;
  }
}

LEVEL *ShowLevelSelector(WIN *W) {
  char levels[MAX_LEVELS][256];
  int num_levels = ScanLevels(levels);

  if (num_levels == 1) {
    return LoadLevel(levels[0]);
  }

  int highlight = 0;
  WINDOW *win = W->window;
  nodelay(win, FALSE);

  while (1) {
    wclear(win);
    box(win, 0, 0);

    const char *title = "=== SELECT LEVEL ===";
    mvwprintw(win, 2, (W->cols - strlen(title)) / 2, "%s", title);

    int visible_items = W->rows - 10;
    int scroll_offset = (highlight >= visible_items) ? 
                        (highlight - visible_items + 1) : 0;

    RenderLevelList(W, levels, num_levels, highlight, scroll_offset);

    mvwprintw(win, W->rows - 2, (W->cols - strlen(instructions)) / 2, "%s",
              instructions);
    wrefresh(win);

    int result = HandleLevelInput(wgetch(win), &highlight, num_levels);
    if (result == 1) {
      return LoadLevel(levels[highlight]);
    } else if (result == -1) {
      return NULL;
    }
  }

  return NULL;
}

static void DisplayRankings(WIN *W, RANKING_ENTRY rankings[], int num_rankings) {
  if (num_rankings == 0) {
    const char *no_rankings = "No rankings yet for this level";
    mvwprintw(W->window, 8, (W->cols - strlen(no_rankings)) / 2, "%s",
              no_rankings);
    return;
  }

  mvwprintw(W->window, 6, 5, "Rank  Final Score  Totems  Lives  Time Left");
  mvwprintw(W->window, 7, 5, "----  -----------  ------  -----  ---------");

  for (int i = 0; i < num_rankings && i < 10; i++) {
    mvwprintw(W->window, 8 + i, 5, "#%-3d  %11d  %6d  %5d  %9d", i + 1,
              rankings[i].final_score, rankings[i].totems_score,
              rankings[i].lives_left, rankings[i].time_left);
  }
}

static int HandleRankingInput(int c, int *current_level, int num_levels) {
  switch (c) {
  case LEFT:
    *current_level = (*current_level - 1 + num_levels) % num_levels;
    return 0;
  case RIGHT:
    *current_level = (*current_level + 1) % num_levels;
    return 0;
  case QUIT:
    return 1;
  default:
    return 0;
  }
}

static void RenderRankingHeader(WIN *W, int current_level,
                                char levels[][256]) {
  const char *title = "=== GAME RANKING ===";
  mvwprintw(W->window, 2, (W->cols - strlen(title)) / 2, "%s", title);

  char *level_name = GetLevelName(levels[current_level]);
  char level_title[100];
  snprintf(level_title, sizeof(level_title), "Level: %s", level_name);
  mvwprintw(W->window, 4, (W->cols - strlen(level_title)) / 2, "%s",
            level_title);
}

void ShowRanking(WIN *W) {
  WINDOW *win = W->window;
  char levels[MAX_LEVELS][256];
  int num_levels = ScanLevels(levels);

  nodelay(win, FALSE);

  if (num_levels == 0) {
    CleanWin(W, 1);
    const char *msg = "No levels found";
    mvwprintw(win, W->rows / 2, (W->cols - strlen(msg)) / 2, "%s", msg);
    wrefresh(win);
    wgetch(win);
    return;
  }

  int current_level = 0;

  while (1) {
    CleanWin(W, 1);
    RenderRankingHeader(W, current_level, levels);

    char *level_name = GetLevelName(levels[current_level]);
    RANKING_ENTRY rankings[MAX_RANKINGS_PER_LEVEL];
    int num_rankings =
        GetLevelRankings(level_name, rankings, MAX_RANKINGS_PER_LEVEL);

    DisplayRankings(W, rankings, num_rankings);

    const char *nav = "Use [A/D] to change levels, [Q] to exit";
    mvwprintw(win, W->rows - 2, (W->cols - strlen(nav)) / 2, "%s", nav);

    wrefresh(win);

    if (HandleRankingInput(wgetch(win), &current_level, num_levels)) {
      return;
    }
  }
}

static int GetEntityColor(const char symbol) {
  switch (symbol) {
  case '@':
    return PLAYER_COLOR;
  case 'T':
    return TOTEM_COLOR;
  case 'E':
    return ENEMY_COLOR;
  case '^':
    return SPIKE_COLOR;
  case 'F':
    return FIREJET_COLOR;
  case 'P':
    return TELEPORT_PAD_COLOR;
  case '~':
    return STICKY_FLOOR_COLOR;
  case ',':
    return TALL_GRASS_COLOR;
  case 'S':
    return POWERUP_SHIELD_COLOR;
  case 'D':
    return POWERUP_DOUBLE_COLOR;
  case '+':
    return POWERUP_LIFE_COLOR;
  default:
    break;
  }

  return -1; // No color
}

static void RenderLineWithColors(WINDOW *win, int y, int x, const char *line,
                                 int max_width) {
  int current_x = x;
  const char *current = line;

  while (*current && (current_x - x) < max_width) {
    int found_entity = FALSE;

    // Entity symbols are patterns like [@], [T], [E], etc.
    if (current[0] == '[' && current[1] != '\0' && current[2] == ']') {
      int color = GetEntityColor(current[1]);

      // Special case: differentiate between FIREJET and TIME FREEZE power-up
      if (color == FIREJET_COLOR && strstr(line, "[F] TIME FREEZE") != NULL) {
        color = POWERUP_FREEZE_COLOR;
      }

      if (color > 0) {
        wattron(win, COLOR_PAIR(color));
        mvwaddch(win, y, current_x, *"[");
        mvwaddch(win, y, current_x + 1, current[1]);
        mvwaddch(win, y, current_x + 2, *"]");
        wattroff(win, COLOR_PAIR(color));
        
        current += 3;
        current_x += 3;
        found_entity = TRUE;
      }
    }

    if (!found_entity) {
      // Regular character - print it at specific coordinate
      mvwaddch(win, y, current_x, *current);
      current++;
      current_x++;
    }
  }
}

void ShowInstructions(WIN *W) {
  WINDOW *win = W->window;
  nodelay(win, FALSE);

  const int total_lines =
      sizeof(GAME_INSTRUCTIONS) / sizeof(GAME_INSTRUCTIONS[0]);

  int scroll_offset = 0;
  int visible_lines = W->rows - 5;
  int max_scroll = MAX(0, total_lines - visible_lines);

  int c;
  while (1) {
    wclear(win);
    box(win, 0, 0);

    // Display visible portion of instructions with dynamically colored entities
    for (int i = 0; i < visible_lines && (scroll_offset + i) < total_lines;
         i++) {
      const char *line = GAME_INSTRUCTIONS[scroll_offset + i];
      RenderLineWithColors(win, i + 2, 2, line, W->cols - 4);
    }

    // Show scroll indicator if there's more content
    if (scroll_offset > 0) {
      mvwprintw(win, 1, W->cols - 15, "^ More above");
    }
    if (scroll_offset < max_scroll) {
      mvwprintw(win, W->rows - 3, W->cols - 15, "v More below");
    }

    // Fixed navigation info at bottom
    const char *nav_info = "Use [W/S] to scroll, [Q] to return to menu";
    mvwprintw(win, W->rows - 2, (W->cols - strlen(nav_info)) / 2, "%s",
              nav_info);

    wrefresh(win);

    c = wgetch(win);
    switch (c) {
    case UP:
      if (scroll_offset > 0) {
        scroll_offset--;
      }
      break;
    case DOWN:
      if (scroll_offset < max_scroll) {
        scroll_offset++;
      }
      break;
    case QUIT:
      return;
    default:
      break;
    }
  }
}
