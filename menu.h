#ifndef MENU_H
#define MENU_H

#include "types.h"

// Menu options
#define MENU_NEW_GAME 0
#define MENU_RANKING 1
#define MENU_EXIT 2
#define MENU_ITEMS 3

// Maximum number of levels
#define MAX_LEVELS 50

// Level selector result structure
typedef struct {
  char filename[256];
  int selected;
} LEVEL_SELECT_RESULT;

// Display main menu and return selected option
int ShowMainMenu(WINDOW *screenwin);

// Display level selector and return selected level filename
LEVEL_SELECT_RESULT ShowLevelSelector(WINDOW *screenwin);

// Show ranking screen (placeholder for now)
void ShowRanking(WINDOW *screenwin);

// Scan levels directory and return list of level files
int ScanLevels(char levels[][256]);

#endif // MENU_H
