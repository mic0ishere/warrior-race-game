#include "menu.h"
#include <dirent.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Main menu items
static const char *menu_items[MENU_ITEMS] = {"New Game", "View Ranking",
                                              "Exit"};

// Comparison function for qsort
static int compare_strings(const void *a, const void *b) {
  return strcmp((const char *)a, (const char *)b);
}

int ShowMainMenu(WINDOW *screenwin) {
  int highlight = 0;
  int choice = -1;
  int c;
  int max_rows, max_cols;

  curs_set(0); // Hide cursor
  keypad(screenwin, TRUE);
  nodelay(screenwin, FALSE);

  while (1) {
    // Get actual terminal dimensions
    getmaxyx(screenwin, max_rows, max_cols);
    
    wclear(screenwin);
    box(screenwin, 0, 0);

    // Title
    mvwprintw(screenwin, 2, (max_cols - 20) / 2, "=== WARRIOR RACE ===");
    mvwprintw(screenwin, 3, (max_cols - 30) / 2, "Terminal Roguelike Adventure");

    // Menu items
    for (int i = 0; i < MENU_ITEMS; i++) {
      if (i == highlight) {
        wattron(screenwin, A_REVERSE);
      }
      mvwprintw(screenwin, 6 + i * 2, (max_cols - 20) / 2, "%s", menu_items[i]);
      if (i == highlight) {
        wattroff(screenwin, A_REVERSE);
      }
    }

    // Instructions
    mvwprintw(screenwin, max_rows - 3, (max_cols - 40) / 2,
              "Use arrow keys or w/s to navigate");
    mvwprintw(screenwin, max_rows - 2, (max_cols - 30) / 2, "Press Enter to select");

    wrefresh(screenwin);

    c = wgetch(screenwin);

    switch (c) {
    case KEY_UP:
    case 'w':
    case 'W':
      highlight = (highlight - 1 + MENU_ITEMS) % MENU_ITEMS;
      break;
    case KEY_DOWN:
    case 's':
    case 'S':
      highlight = (highlight + 1) % MENU_ITEMS;
      break;
    case 10: // Enter key
    case KEY_ENTER:
      choice = highlight;
      return choice;
    case 'q':
    case 'Q':
      return MENU_EXIT;
    default:
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

LEVEL_SELECT_RESULT ShowLevelSelector(WINDOW *screenwin) {
  char levels[MAX_LEVELS][256];
  int num_levels = ScanLevels(levels);
  LEVEL_SELECT_RESULT result;
  int max_rows, max_cols;
  result.selected = 0;
  result.filename[0] = '\0';

  // Get actual terminal dimensions
  getmaxyx(screenwin, max_rows, max_cols);

  if (num_levels == 0) {
    wclear(screenwin);
    box(screenwin, 0, 0);
    mvwprintw(screenwin, max_rows / 2, (max_cols - 30) / 2,
              "No levels found in levels/ directory!");
    mvwprintw(screenwin, max_rows / 2 + 2, (max_cols - 20) / 2,
              "Press any key to return");
    wrefresh(screenwin);
    nodelay(screenwin, FALSE);
    wgetch(screenwin);
    return result;
  }

  int highlight = 0;
  int c;

  curs_set(0);
  keypad(screenwin, TRUE);
  nodelay(screenwin, FALSE);

  while (1) {
    // Get actual terminal dimensions in case of resize
    getmaxyx(screenwin, max_rows, max_cols);
    
    wclear(screenwin);
    box(screenwin, 0, 0);

    // Title
    mvwprintw(screenwin, 2, (max_cols - 20) / 2, "=== SELECT LEVEL ===");
    mvwprintw(screenwin, 3, (max_cols - 30) / 2, "Found %d level(s)", num_levels);

    // Level list
    int start_y = 6;
    int visible_items = max_rows - 10;
    int scroll_offset = 0;

    if (highlight >= visible_items) {
      scroll_offset = highlight - visible_items + 1;
    }

    for (int i = scroll_offset;
         i < num_levels && i < scroll_offset + visible_items; i++) {
      if (i == highlight) {
        wattron(screenwin, A_REVERSE);
      }

      // Extract just the filename without path and extension for display
      char display_name[256];
      const char *filename = levels[i];
      const char *basename = strrchr(filename, '/');
      if (basename) {
        basename++; // Skip the '/'
      } else {
        basename = filename;
      }
      // Safe string copy - leave room for null terminator
      strncpy(display_name, basename, sizeof(display_name) - 1);
      display_name[sizeof(display_name) - 1] = '\0';

      // Remove .txt extension
      char *ext = strstr(display_name, ".txt");
      if (ext) {
        *ext = '\0';
      }

      mvwprintw(screenwin, start_y + (i - scroll_offset), (max_cols - 40) / 2,
                "%s", display_name);

      if (i == highlight) {
        wattroff(screenwin, A_REVERSE);
      }
    }

    // Instructions
    mvwprintw(screenwin, max_rows - 3, (max_cols - 40) / 2,
              "Use arrow keys or w/s to navigate");
    mvwprintw(screenwin, max_rows - 2, (max_cols - 30) / 2,
              "Enter to select, Q to cancel");

    wrefresh(screenwin);

    c = wgetch(screenwin);

    switch (c) {
    case KEY_UP:
    case 'w':
    case 'W':
      highlight = (highlight - 1 + num_levels) % num_levels;
      break;
    case KEY_DOWN:
    case 's':
    case 'S':
      highlight = (highlight + 1) % num_levels;
      break;
    case 10: // Enter key
    case KEY_ENTER:
      // Safe string copy - ensure null termination
      strncpy(result.filename, levels[highlight], sizeof(result.filename) - 1);
      result.filename[sizeof(result.filename) - 1] = '\0';
      result.selected = 1;
      return result;
    case 'q':
    case 'Q':
      result.selected = 0;
      return result;
    default:
      break;
    }
  }

  return result;
}

void ShowRanking(WINDOW *screenwin) {
  int max_rows, max_cols;
  
  // Get actual terminal dimensions
  getmaxyx(screenwin, max_rows, max_cols);
  
  wclear(screenwin);
  box(screenwin, 0, 0);

  mvwprintw(screenwin, 2, (max_cols - 20) / 2, "=== RANKING ===");
  mvwprintw(screenwin, max_rows / 2, (max_cols - 40) / 2,
            "Ranking system coming soon!");
  mvwprintw(screenwin, max_rows / 2 + 2, (max_cols - 35) / 2,
            "Check back in the next update.");
  mvwprintw(screenwin, max_rows - 2, (max_cols - 20) / 2, "Press any key to return");

  wrefresh(screenwin);
  nodelay(screenwin, FALSE);
  wgetch(screenwin);
}
