#ifndef WINDOW_H
#define WINDOW_H

#include "types.h"

// Initialize ncurses and color system
WINDOW *InitScreen(void);

// Clean a window by filling with spaces
void CleanWin(WIN *W, int bo);

// Initialize a new window
WIN *InitWin(WINDOW *parent, int rows, int cols, int y, int x, int color,
             int bo, int is_debug);

// Initialize all game windows
int InitAllWindows(WINDOW *screenwin, WINDOWS *windows);

// Cleanup all windows
void CleanupWindows(WINDOWS *windows);

// Cleanup and close a single window
void CleanupWin(WIN *W);

// Display end game message
void EndGame(WIN *W, char *msg, int color_pair);

// Display status bar
void ShowStatus(WIN *W, GAMESTATE *state);

// Display debug information
void ShowDebug(WIN *W, const char *msg);

// Show life loss reason in status window
void ShowLifeLoss(WIN *W, LIFE_LOST_STATUS life_loss);

// Render alert message in window
void ShowAlert(WIN *W, const char *msg);

// Convert level coordinates to screen coordinates
void LevelToScreenCoords(WIN *W, int level_x, int level_y, GAMESTATE *state,
                         int *screen_x, int *screen_y);

// Safely print entity on screen (takes level coordinates)
void SafePrintEntity(WIN *W, int level_x, int level_y, const char *symbol,
                     GAMESTATE *state, int color_pair);

// Render the play area
void RenderPlayArea(WIN *W, GAMESTATE *state);

#endif // WINDOW_H
