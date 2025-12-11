#ifndef MENU_H
#define MENU_H

#include "types.h"

// Menu options
#define MENU_NEW_GAME 0
#define MENU_RANKING 1
#define MENU_EXIT 2
#define MENU_ITEMS 3

// Maximum number of levels
#define MAX_LEVELS 30

// Display main menu and return selected option
int ShowMainMenu(WIN *screenwin);

// Extract just the filename without path and extension for display
char *GetLevelName(const char *filename);

// Display level selector and return selected level filename
LEVEL *ShowLevelSelector(WIN *screenwin);

// Show ranking screen (placeholder for now)
void ShowRanking(WIN *screenwin);

// Scan levels directory and return list of level files
int ScanLevels(char levels[][256]);

#endif // MENU_H
