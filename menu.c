#include "menu.h"
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
static const char *menu_items[MENU_ITEMS] = {"New Game", "Instructions",
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

  // Scan available levels
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
  int c;

  while (1) {
    CleanWin(W, 1);

    const char *title = "=== GAME RANKING ===";
    mvwprintw(win, 2, (W->cols - strlen(title)) / 2, "%s", title);

    // Display current level name
    char *level_name = GetLevelName(levels[current_level]);
    char level_title[100];
    snprintf(level_title, sizeof(level_title), "Level: %s", level_name);
    mvwprintw(win, 4, (W->cols - strlen(level_title)) / 2, "%s", level_title);

    // Load rankings for current level
    RANKING_ENTRY rankings[MAX_RANKINGS_PER_LEVEL];
    int num_rankings =
        GetLevelRankings(level_name, rankings, MAX_RANKINGS_PER_LEVEL);

    if (num_rankings == 0) {
      const char *no_rankings = "No rankings yet for this level";
      mvwprintw(win, 8, (W->cols - strlen(no_rankings)) / 2, "%s", no_rankings);
    } else {
      // Display header
      mvwprintw(win, 6, 5, "Rank  Final Score  Totems  Lives  Time Left");
      mvwprintw(win, 7, 5, "----  -----------  ------  -----  ---------");

      // Display rankings
      for (int i = 0; i < num_rankings && i < 10; i++) {
        mvwprintw(win, 8 + i, 5, "#%-3d  %11d  %6d  %5d  %9d", i + 1,
                  rankings[i].final_score, rankings[i].totems_score,
                  rankings[i].lives_left, rankings[i].time_left);
      }
    }

    const char *nav = "Use [A/D] to change levels, [Q] to exit";
    mvwprintw(win, W->rows - 2, (W->cols - strlen(nav)) / 2, "%s", nav);

    wrefresh(win);

    c = wgetch(win);

    switch (c) {
    case LEFT:
      current_level = (current_level - 1 + num_levels) % num_levels;
      break;
    case RIGHT:
      current_level = (current_level + 1) % num_levels;
      break;
    case QUIT:
      return;
    default:
      break;
    }
  }
}

void ShowInstructions(WIN *W) {
  WINDOW *win = W->window;
  nodelay(win, FALSE);

  // Define all instruction lines
  const char *instruction_lines[] = {
    "=== WARRIOR RACE - GAME INSTRUCTIONS ===",
    "",
    "OBJECTIVE:",
    "  Collect totems to earn points and reach the win score before time",
    "  runs out! Navigate the level, avoid enemies and obstacles, and use",
    "  power-ups strategically.",
    "",
    "CONTROLS:",
    "  W/A/S/D - Move player (Up/Left/Down/Right)",
    "  P - Pause game",
    "  Q - Quit to menu",
    "",
    "GAME MECHANICS:",
    "  * Collect totems (@) to earn points",
    "  * Reach WIN_SCORE points before time expires",
    "  * Each life lost gives you a brief invulnerability period",
    "  * Final score includes bonuses for remaining lives and time",
    "",
    "=== ENEMIES ===",
    "",
    "  [E] PATROL - Moves back and forth along a set path",
    "      Behavior: Predictable movement between two points",
    "",
    "  [E] RANDOM - Wanders randomly within bounds",
    "      Behavior: Unpredictable movement pattern",
    "",
    "  [E] CHASER - Pursues player when in range",
    "      Behavior: Actively hunts player within chase radius",
    "      Note: Blue when sleeping, bright red when chasing",
    "",
    "  [E] TELEPORTER - Randomly teleports around level",
    "      Behavior: Dangerous and unpredictable positioning",
    "      Note: Flashes magenta before teleporting",
    "",
    "=== AGGRESSIVE OBSTACLES ===",
    "",
    "  [^] SPIKE - Harms player from specific direction",
    "      Behavior: Activates at regular intervals",
    "      Danger: Only harmful when moving INTO spike direction",
    "",
    "  [#] FIREJET - Shoots fire in straight line",
    "      Behavior: Emits fire at set intervals",
    "      Danger: Fire travels multiple tiles in one direction",
    "",
    "=== PASSIVE OBSTACLES ===",
    "",
    "  [T] TELEPORT PAD - Instant transport to linked pad",
    "      Effect: Jump to destination pad (2 second cooldown)",
    "",
    "  [~] STICKY FLOOR - Slows player movement",
    "      Effect: Movement 4x slower",
    "",
    "  [,] TALL GRASS - Reduces vision range",
    "      Effect: Only see 2 tiles around player while inside",
    "",
    "=== POWER-UPS ===",
    "",
    "  [S] SHIELD - Negates next enemy collision",
    "      Effect: One-time protection from enemy damage",
    "",
    "  [D] DOUBLE POINTS - Double totem values",
    "      Effect: Lasts 10 seconds",
    "",
    "  [+] EXTRA LIFE - Gain one additional life",
    "      Effect: Instant life bonus",
    "",
    "  [F] TIME FREEZE - Stops all enemy movement",
    "      Effect: Lasts 5 seconds",
    "",
    "TIPS:",
    "  * Plan your route to collect totems efficiently",
    "  * Watch enemy patterns before engaging",
    "  * Use power-ups at critical moments",
    "  * Teleport pads can be shortcuts or escapes",
    "  * Avoid sticky floors when being chased"
  };

  const int total_lines = sizeof(instruction_lines) / sizeof(instruction_lines[0]);
  int scroll_offset = 0;
  int visible_lines = W->rows - 5; // Leave room for borders and fixed navigation
  int max_scroll = MAX(0, total_lines - visible_lines);

  int c;
  while (1) {
    wclear(win);
    box(win, 0, 0);

    // Display visible portion of instructions with colors
    for (int i = 0; i < visible_lines && (scroll_offset + i) < total_lines; i++) {
      const char *line = instruction_lines[scroll_offset + i];
      int line_y = 2 + i;
      
      // Check if line contains colored entities and render them with colors
      if (strstr(line, "[E]") && strstr(line, "PATROL")) {
        mvwprintw(win, line_y, 2, "  ");
        wattron(win, COLOR_PAIR(ENEMY_COLOR));
        wprintw(win, "[E]");
        wattroff(win, COLOR_PAIR(ENEMY_COLOR));
        wprintw(win, " PATROL - Moves back and forth along a set path");
      } else if (strstr(line, "[E]") && strstr(line, "RANDOM")) {
        mvwprintw(win, line_y, 2, "  ");
        wattron(win, COLOR_PAIR(ENEMY_COLOR));
        wprintw(win, "[E]");
        wattroff(win, COLOR_PAIR(ENEMY_COLOR));
        wprintw(win, " RANDOM - Wanders randomly within bounds");
      } else if (strstr(line, "[E]") && strstr(line, "CHASER")) {
        mvwprintw(win, line_y, 2, "  ");
        wattron(win, COLOR_PAIR(CHASER_SLEEPING_COLOR));
        wprintw(win, "[E]");
        wattroff(win, COLOR_PAIR(CHASER_SLEEPING_COLOR));
        wprintw(win, " CHASER - Pursues player when in range");
      } else if (strstr(line, "[E]") && strstr(line, "TELEPORTER")) {
        mvwprintw(win, line_y, 2, "  ");
        wattron(win, COLOR_PAIR(ENEMY_COLOR));
        wprintw(win, "[E]");
        wattroff(win, COLOR_PAIR(ENEMY_COLOR));
        wprintw(win, " TELEPORTER - Randomly teleports around level");
      } else if (strstr(line, "[^]")) {
        mvwprintw(win, line_y, 2, "  ");
        wattron(win, COLOR_PAIR(SPIKE_COLOR));
        wprintw(win, "[^]");
        wattroff(win, COLOR_PAIR(SPIKE_COLOR));
        wprintw(win, " SPIKE - Harms player from specific direction");
      } else if (strstr(line, "[#]")) {
        mvwprintw(win, line_y, 2, "  ");
        wattron(win, COLOR_PAIR(FIREJET_COLOR));
        wprintw(win, "[#]");
        wattroff(win, COLOR_PAIR(FIREJET_COLOR));
        wprintw(win, " FIREJET - Shoots fire in straight line");
      } else if (strstr(line, "[T]")) {
        mvwprintw(win, line_y, 2, "  ");
        wattron(win, COLOR_PAIR(TELEPORT_PAD_COLOR));
        wprintw(win, "[T]");
        wattroff(win, COLOR_PAIR(TELEPORT_PAD_COLOR));
        wprintw(win, " TELEPORT PAD - Instant transport to linked pad");
      } else if (strstr(line, "[~]")) {
        mvwprintw(win, line_y, 2, "  ");
        wattron(win, COLOR_PAIR(STICKY_FLOOR_COLOR));
        wprintw(win, "[~]");
        wattroff(win, COLOR_PAIR(STICKY_FLOOR_COLOR));
        wprintw(win, " STICKY FLOOR - Slows player movement");
      } else if (strstr(line, "[,]")) {
        mvwprintw(win, line_y, 2, "  ");
        wattron(win, COLOR_PAIR(TALL_GRASS_COLOR));
        wprintw(win, "[,]");
        wattroff(win, COLOR_PAIR(TALL_GRASS_COLOR));
        wprintw(win, " TALL GRASS - Reduces vision range");
      } else if (strstr(line, "[S]")) {
        mvwprintw(win, line_y, 2, "  ");
        wattron(win, COLOR_PAIR(POWERUP_SHIELD_COLOR));
        wprintw(win, "[S]");
        wattroff(win, COLOR_PAIR(POWERUP_SHIELD_COLOR));
        wprintw(win, " SHIELD - Negates next enemy collision");
      } else if (strstr(line, "[D]")) {
        mvwprintw(win, line_y, 2, "  ");
        wattron(win, COLOR_PAIR(POWERUP_DOUBLE_COLOR));
        wprintw(win, "[D]");
        wattroff(win, COLOR_PAIR(POWERUP_DOUBLE_COLOR));
        wprintw(win, " DOUBLE POINTS - Double totem values");
      } else if (strstr(line, "[+]")) {
        mvwprintw(win, line_y, 2, "  ");
        wattron(win, COLOR_PAIR(POWERUP_LIFE_COLOR));
        wprintw(win, "[+]");
        wattroff(win, COLOR_PAIR(POWERUP_LIFE_COLOR));
        wprintw(win, " EXTRA LIFE - Gain one additional life");
      } else if (strstr(line, "[F]")) {
        mvwprintw(win, line_y, 2, "  ");
        wattron(win, COLOR_PAIR(POWERUP_FREEZE_COLOR));
        wprintw(win, "[F]");
        wattroff(win, COLOR_PAIR(POWERUP_FREEZE_COLOR));
        wprintw(win, " TIME FREEZE - Stops all enemy movement");
      } else {
        mvwprintw(win, line_y, 2, "%.*s", W->cols - 4, line);
      }
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
    mvwprintw(win, W->rows - 2, (W->cols - strlen(nav_info)) / 2, "%s", nav_info);

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
