#include "menu.h"
#include "level.h"
#include <dirent.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Main menu items
static const char *menu_items[MENU_ITEMS] = {"New Game", "View Ranking",
                                             "Exit"};

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
    // Check if file ends with .txt
    size_t len = strlen(entry->d_name);
    if (len > 4 && strcmp(entry->d_name + len - 4, ".txt") == 0) {
      snprintf(levels[count], 256, "levels/%s", entry->d_name);
      count++;
    }
  }

  closedir(dir);

  // Sort levels alphabetically using qsort
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

LEVEL *ShowLevelSelector(WIN *W) {
  char levels[MAX_LEVELS][256];
  int num_levels = ScanLevels(levels);

  if (num_levels == 1) {
    return LoadLevel(levels[0]);
  }

  int highlight = 0;
  int c;

  WINDOW *win = W->window;

  // Wait for user input
  nodelay(win, FALSE);

  while (1) {
    wclear(win);
    box(win, 0, 0);

    const char *title = "=== SELECT LEVEL ===";
    mvwprintw(win, 2, (W->cols - strlen(title)) / 2, "%s", title);

    // Level list
    int start_y = 5;
    int visible_items = W->rows - 10;
    int scroll_offset = 0;
    const int column_start = (W->cols) / 2 - 16;

    if (highlight >= visible_items) {
      scroll_offset = highlight - visible_items + 1;
    }

    for (int i = scroll_offset;
         i < num_levels && i <= scroll_offset + visible_items; i++) {
      if (i == highlight) {
        wattron(win, A_REVERSE);
      }

      char *level_name = GetLevelName(levels[i]);

      mvwprintw(win, start_y + (i - scroll_offset), column_start, "%d. %s",
                i + 1, level_name);

      if (i == highlight) {
        wattroff(win, A_REVERSE);
      }
    }

    if (scroll_offset + visible_items < num_levels - 1) {
      // Draw an empty line to clear previous content and show '... more levels'
      mvwhline(win, start_y + visible_items, 1, ' ', W->cols - 2);
      mvwprintw(win, start_y + visible_items, column_start,
                "... %d more levels\t\t\t",
                num_levels - (scroll_offset + visible_items));
    }

    // Display instructions
    mvwprintw(win, W->rows - 2, (W->cols - strlen(instructions)) / 2, "%s",
              instructions);

    wrefresh(win);

    c = wgetch(win);

    switch (c) {
    case UP:
      highlight = (highlight - 1 + num_levels) % num_levels;
      break;
    case DOWN:
      highlight = (highlight + 1) % num_levels;
      break;
    case CONFIRM:
      return LoadLevel(levels[highlight]);
    case QUIT:
      return NULL;
    default:
      if (c >= '1' && c <= '0' + num_levels) {
        return LoadLevel(levels[c - '1']);
      }

      break;
    }
  }

  return NULL;
}

void ShowRanking(WIN *W) {
  WINDOW *win = W->window;

  wclear(win);
  box(win, 0, 0);

  const char *title = "=== GAME RANKING ===";
  mvwprintw(win, 2, (W->cols - strlen(title)) / 2, "%s", title);

  const char *instructions = "Press any key to return";
  mvwprintw(win, W->rows - 2, (W->cols - strlen(instructions)) / 2, "%s",
            instructions);

  wrefresh(win);

  // Wait for user input
  nodelay(win, FALSE);
  wgetch(win);
}
