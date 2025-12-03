#ifndef TYPES_H
#define TYPES_H

#include <ncurses.h>

// Keyboard controls
#define QUIT 'q'
#define UP 'w'
#define DOWN 's'
#define LEFT 'a'
#define RIGHT 'd'
#define NOKEY ERR

// Timing and speed
#define FRAMES_PER_SECOND 20
#define LEVEL_FILE "level1.txt"

// Color pair identifiers
#define PLAY_COLOR 1
#define STAT_COLOR 2
#define DEBUG_COLOR 3
#define END_WIN_COLOR 4
#define END_LOSE_COLOR 5
#define ENEMY_COLOR 6
#define TOTEM_COLOR 7
#define OBSTACLE_COLOR 8
#define PLAYER_COLOR 9

// Window dimensions and position
#define BORDER 1
#define COLS 80
#define ROWS 20
#define STAT_ROWS 5
#define OFFY 2
#define OFFX 5

// Game status codes
#define CONTINUE 0
#define GAME_TIMEOUT 1
#define OUT_OF_LIVES 2
#define USER_QUIT 3
#define WIN_POINTS 10

// Boolean values
#define TRUE 1
#define FALSE 0

// Enemy direction
#define DIR_FORWARD 1
#define DIR_BACKWARD -1

// Limits
#define MAX_DEBUG_LEN (COLS - 3)
#define MAX_LINE_LEN 256

// Utility macros
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// Frame time in milliseconds
extern const int FRAME_TIME_MS;

// Window structure
typedef struct {
  WINDOW *window;
  int x, y;
  int rows, cols;
  int color;
} WIN;

// Position structure
typedef struct {
  int x, y;
} POSITION;

// Totem structure
typedef struct {
  POSITION pos;
  int points;
  int is_collected;
} TOTEM;

// Obstacle structure
typedef struct {
  POSITION pos1;
  POSITION pos2;
} OBSTACLE;

// Enemy type enumeration
typedef enum {
  ENEMY_PATROL_HORIZONTAL,
  ENEMY_PATROL_VERTICAL,
  ENEMY_TYPE_UNKNOWN
} ENEMY_TYPE;

// Enemy structure
typedef struct {
  POSITION pos1;
  POSITION pos2;
  POSITION current_pos;
  ENEMY_TYPE type;
  int direction;
  int speed;
} ENEMY;

// Level structure
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

// Game state structure
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

// Windows group structure
typedef struct {
  WIN *playwin;
  WIN *statwin;
  WIN *debugwin;
} WINDOWS;

#endif // TYPES_H
