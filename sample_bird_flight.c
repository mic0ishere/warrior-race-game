/*
        ===================================================================
        FLYING BIRD GAME - Simple Template for Learning ncurses
        ===================================================================

        Based on win1.c framework by MM
        Educational code for beginners learning C and ncurses library

        CONTROLS:
          SPACE - Reverse flight direction (change dx and dy to opposite)
          q     - Quit the game

        GAME MECHANICS:
          - Bird (*) flies automatically with constant velocity
          - Bird bounces off walls (borders of the play window)
          - Two windows: play area (top) and status bar (bottom)

        COMPILATION:
          gcc -o bird_flight bird_flight.c -lncurses

        ===================================================================
*/

#include <ncurses.h> // Text-based UI library
#include <stdio.h>   // Standard input/output (printf, fprintf)
#include <stdlib.h>  // Standard library (malloc, free, exit)
#include <string.h>  // String operations (memset, strcpy)
#include <unistd.h>  // Unix standard (usleep for timing)

//------------------------------------------------
//------------  CONFIGURATION CONSTANTS ----------
//------------------------------------------------

// Keyboard controls
#define QUIT 'q'    // Key to quit the game
#define REVERSE ' ' // Key to reverse bird direction (SPACE)
#define NOKEY ERR   // ncurses constant for "no key pressed"

// Timing and speed
#define FRAME_TIME 100 // Milliseconds per frame (100ms = 0.1 sec)
#define BIRD_SPEED 1   // Bird moves 1 cell per frame

// Color pair identifiers (used with ncurses color system)
#define MAIN_COLOR 1 // Main window color
#define STAT_COLOR 2 // Status bar color
#define PLAY_COLOR 3 // Play area color
#define BIRD_COLOR 4 // Bird color

// Window dimensions and position
#define BORDER 1 // Border width (in characters)
#define ROWS 20  // Play window height (rows)
#define COLS 80  // Play window width (columns)
#define OFFY 2   // Y offset from top of screen
#define OFFX 5   // X offset from left of screen

//------------------------------------------------
//------------  DATA STRUCTURES ------------------
//------------------------------------------------

/*
        WIN structure - represents a window on the screen

        Fields:
          window - pointer to ncurses WINDOW structure
          x, y   - top-left corner position on screen
          rows, cols - dimensions of the window
          color  - color pair identifier for this window
*/
typedef struct {
  WINDOW *window; // ncurses window pointer
  int x, y;       // position on screen
  int rows, cols; // size of window
  int color;      // color scheme
} WIN;

/*
        BIRD structure - represents the flying bird

        Fields:
          win    - pointer to the window where bird flies
          x, y   - current position of the bird
          dx, dy - velocity direction (-1, 0, or 1 for each axis)
                   dx=1 means moving right, dx=-1 means moving left
                   dy=1 means moving down, dy=-1 means moving up
          speed  - how many cells bird moves per frame
          symbol - character used to draw the bird (usually '*')
          color  - color pair identifier
*/
typedef struct {
  WIN *win;    // window containing the bird
  int x, y;    // current position
  int dx, dy;  // velocity direction vector
  int speed;   // movement speed
  char symbol; // character to display
  int color;   // color scheme
} BIRD;

//------------------------------------------------
//------------  NCURSES INITIALIZATION -----------
//------------------------------------------------

/*
        Start() - Initialize ncurses library and color system

        This function:
          1. Initializes ncurses (initscr)
          2. Sets up color pairs for different UI elements
          3. Configures terminal for game (noecho, invisible cursor)

        Returns: pointer to main window
*/
WINDOW *Start() {
  WINDOW *win;

  // Initialize ncurses - sets up terminal for text UI
  if ((win = initscr()) == NULL) {
    fprintf(stderr, "Error initialising ncurses.\n");
    exit(EXIT_FAILURE);
  }

  // Initialize color system
  start_color();

  // Define color pairs: init_pair(ID, FOREGROUND, BACKGROUND)
  init_pair(MAIN_COLOR, COLOR_WHITE, COLOR_BLACK);
  init_pair(PLAY_COLOR, COLOR_CYAN, COLOR_BLACK);
  init_pair(STAT_COLOR, COLOR_YELLOW, COLOR_BLUE);
  init_pair(BIRD_COLOR, COLOR_RED, COLOR_BLACK);

  // Don't echo typed characters to screen
  noecho();

  // Make cursor invisible (curs_set: 0=invisible, 1=normal, 2=very visible)
  curs_set(0);

  return win;
}

//------------------------------------------------
//------------  WINDOW MANAGEMENT FUNCTIONS ------
//------------------------------------------------

/*
        CleanWin() - Clear window by filling with spaces

        Parameters:
          W  - pointer to WIN structure
          bo - border flag: 1=draw border, 0=no border

        This function fills the window with space characters,
        effectively clearing any previous content.
*/
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

/*
        InitWin() - Create and initialize a new window

        Parameters:
          parent - parent ncurses WINDOW
          rows, cols - dimensions of new window
          y, x   - position on screen
          color  - color pair identifier
          bo     - border flag (1=draw border, 0=no border)
          delay  - 0=non-blocking input, 1=blocking input

        Returns: pointer to newly created WIN structure
*/
WIN *InitWin(WINDOW *parent, int rows, int cols, int y, int x, int color,
             int bo, int delay) {
  // Allocate memory for WIN structure
  WIN *W = (WIN *)malloc(sizeof(WIN));

  // Store window properties
  W->x = x;
  W->y = y;
  W->rows = rows;
  W->cols = cols;
  W->color = color;

  // Create ncurses subwindow
  W->window = subwin(parent, rows, cols, y, x);

  // Clear the window
  CleanWin(W, bo);

  // Set input mode: delay==0 means non-blocking (for real-time games)
  if (delay == 0)
    nodelay(W->window, TRUE);

  // Display the window
  wrefresh(W->window);

  return W;
}

//------------------------------------------------
//------------  BIRD FUNCTIONS -------------------
//------------------------------------------------

/*
        InitBird() - Create and initialize a bird

        Parameters:
          w      - window where bird will fly
          x, y   - initial position
          dx, dy - initial velocity direction

        Returns: pointer to newly created BIRD structure
*/
BIRD *InitBird(WIN *w, int x, int y, int dx, int dy) {
  // Allocate memory for BIRD structure
  BIRD *b = (BIRD *)malloc(sizeof(BIRD));

  // Set bird properties
  b->win = w;            // window containing bird
  b->x = x;              // initial x position
  b->y = y;              // initial y position
  b->dx = dx;            // direction: -1=left, 1=right
  b->dy = dy;            // direction: -1=up, 1=down
  b->speed = BIRD_SPEED; // movement speed
  b->symbol = '*';       // display character
  b->color = BIRD_COLOR; // color scheme

  return b;
}

/*
        DrawBird() - Draw the bird at its current position

        Parameters:
          b - pointer to BIRD structure
*/
void DrawBird(BIRD *b) {
  // Set bird color
  wattron(b->win->window, COLOR_PAIR(b->color));

  // Draw bird symbol at current position
  mvwprintw(b->win->window, b->y, b->x, "%c", b->symbol);

  // Restore window color
  wattron(b->win->window, COLOR_PAIR(b->win->color));
}

/*
        ClearBird() - Erase the bird from its current position

        Parameters:
          b - pointer to BIRD structure

        This is called before moving the bird to erase its old position.
*/
void ClearBird(BIRD *b) {
  // Overwrite bird with a space character
  mvwprintw(b->win->window, b->y, b->x, " ");
}

/*
        MoveBird() - Update bird position and handle wall bouncing

        Parameters:
          b - pointer to BIRD structure

        Movement algorithm:
          1. Clear bird from old position
          2. Update position: x += dx * speed, y += dy * speed
          3. Check boundaries and reverse direction if hit
          4. Draw bird at new position

        Bouncing logic:
          - When bird hits a wall, only direction (dx/dy) is reversed
          - Bird position is NOT forced back - it naturally bounces
*/
void MoveBird(BIRD *b) {
  // Step 1: Erase bird from old position
  ClearBird(b);

  // Step 2: Check if bird is already at boundary
  // If at boundary, only reverse direction - don't move!
  int at_x_boundary = (b->x <= BORDER) || (b->x >= b->win->cols - BORDER - 1);
  int at_y_boundary = (b->y <= BORDER) || (b->y >= b->win->rows - BORDER - 1);

  // Step 3: Handle horizontal movement
  if (at_x_boundary) {
    // Already at X boundary - just reverse direction if needed
    if (b->x <= BORDER) {
      b->dx = 1; // Change direction to right
    } else if (b->x >= b->win->cols - BORDER - 1) {
      b->dx = -1; // Change direction to left
    }
    // Don't change X position!
  }

  // Calculate new position
  int new_x = b->x + b->dx * b->speed;

  // Check if new position would hit boundary
  if (new_x <= BORDER) {
    b->x = BORDER;
    b->dx = 1;
  } else if (new_x >= b->win->cols - BORDER - 1) {
    b->x = b->win->cols - BORDER - 1;
    b->dx = -1;
  } else {
    b->x = new_x; // Accept new position
  }

  // Step 4: Handle vertical movement
  if (at_y_boundary) {
    // Already at Y boundary - just reverse direction if needed
    if (b->y <= BORDER) {
      b->dy = 1; // Change direction to down
    } else if (b->y >= b->win->rows - BORDER - 1) {
      b->dy = -1; // Change direction to up
    }
    // Don't change Y position!
  }

  // Calculate new position
  int new_y = b->y + b->dy * b->speed;

  // Check if new position would hit boundary
  if (new_y <= BORDER) {
    b->y = BORDER;
    b->dy = 1;
  } else if (new_y >= b->win->rows - BORDER - 1) {
    b->y = b->win->rows - BORDER - 1;
    b->dy = -1;
  } else {
    b->y = new_y; // Accept new position
  }

  // Step 5: Draw bird at new position
  DrawBird(b);
}

/*
        ReverseBird() - Reverse bird's flight direction

        Parameters:
          b - pointer to BIRD structure

        This is called when player presses SPACE key.
        It reverses both dx and dy, making bird fly in opposite direction.
*/
void ReverseBird(BIRD *b) {
  b->dx = -b->dx; // Flip horizontal direction
  b->dy = -b->dy; // Flip vertical direction
}

//------------------------------------------------
//------------  STATUS BAR FUNCTIONS -------------
//------------------------------------------------

/*
        ShowStatus() - Display game information in status bar

        Parameters:
          W - pointer to status window
          b - pointer to BIRD structure

        Displays:
          - Current bird position (x, y)
          - Control instructions
*/
void ShowStatus(WIN *W, BIRD *b) {
  // Set status bar color
  wattron(W->window, COLOR_PAIR(W->color));

  // Draw border around status bar
  box(W->window, 0, 0);

  // Display bird position
  mvwprintw(W->window, 1, 2, "Position: x=%d y=%d", b->x, b->y);

  // Display controls
  mvwprintw(W->window, 1, 30, "[SPACE]=Reverse [Q]=Quit");

  // Update display
  wrefresh(W->window);
}

//------------------------------------------------
//------------  MAIN GAME LOOP -------------------
//------------------------------------------------

/*
        MainLoop() - Main game loop - runs until player quits

        Parameters:
          playwin - play area window
          statwin - status bar window
          bird    - pointer to BIRD structure

        Loop structure:
          1. Read keyboard input (non-blocking)
          2. Process input (quit or reverse)
          3. Move bird automatically
          4. Update status display
          5. Sleep to control frame rate
          6. Repeat
*/
void MainLoop(WIN *playwin, WIN *statwin, BIRD *bird) {
  int ch; // Variable to store key press

  // Infinite loop - runs until player quits
  while (1) {
    // Read keyboard input (non-blocking due to nodelay(TRUE))
    ch = wgetch(statwin->window);

    // Check if player wants to quit
    if (ch == QUIT)
      break;

    // Check if player wants to reverse direction
    if (ch == REVERSE) {
      ReverseBird(bird);
    }

    // Move bird (automatic movement every frame)
    MoveBird(bird);

    // Update status bar with current position
    ShowStatus(statwin, bird);

    // Refresh play window to show changes
    wrefresh(playwin->window);

    // Clear input buffer to avoid key press accumulation
    flushinp();

    // Sleep to control frame rate
    // FRAME_TIME is in milliseconds, usleep needs microseconds
    usleep(FRAME_TIME * 1000);
  }
}

/*
        EndGame() - Display game over message

        Parameters:
          W - window where message is displayed
*/
void EndGame(WIN *W) {
  // Clear the window
  CleanWin(W, 1);

  // Display goodbye message
  mvwprintw(W->window, 1, 2, "Game Over! Thanks for playing!");
  wrefresh(W->window);

  // Wait 2 seconds before closing
  sleep(2);
}

//------------------------------------------------
//------------  MAIN FUNCTION --------------------
//------------------------------------------------

/*
        main() - Program entry point

        Program flow:
          1. Initialize ncurses
          2. Create windows (play area + status bar)
          3. Create bird object
          4. Run main game loop
          5. Clean up and exit
*/
int main() {
  // Step 1: Initialize ncurses and color system
  WINDOW *mainwin = Start();

  // Step 2: Create game windows
  // Play window: 20x80 cells, positioned at (2,5) with border
  WIN *playwin =
      InitWin(mainwin, ROWS, COLS, OFFY, OFFX, PLAY_COLOR, BORDER, 0);

  // Status window: 3x80 cells, positioned below play window
  WIN *statwin =
      InitWin(mainwin, 3, COLS, ROWS + OFFY, OFFX, STAT_COLOR, BORDER, 0);

  // Step 3: Create bird in center, moving diagonally down-right
  // Initial position: (COLS/2, ROWS/2)
  // Initial velocity: dx=1 (right), dy=1 (down)
  BIRD *bird = InitBird(playwin, COLS / 2, ROWS / 2, 1, 1);

  // Step 4: Initial display
  DrawBird(bird);            // Draw bird
  ShowStatus(statwin, bird); // Update status bar
  wrefresh(playwin->window); // Refresh play window

  // Step 5: Run main game loop (returns when player quits)
  MainLoop(playwin, statwin, bird);

  // Step 6: Cleanup - free resources and close ncurses

  // Display game over message
  EndGame(statwin);

  // Delete ncurses windows
  delwin(playwin->window);
  delwin(statwin->window);
  delwin(mainwin);

  // Close ncurses
  endwin();
  refresh();

  // Free allocated memory
  free(bird);
  free(playwin);
  free(statwin);

  return EXIT_SUCCESS;
}