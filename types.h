#ifndef TYPES_H
#define TYPES_H

#include <ncurses.h>

// Keyboard controls
#define QUIT 'q'
#define UP 'w'
#define DOWN 's'
#define LEFT 'a'
#define RIGHT 'd'
#define PAUSE 'p'
#define CONFIRM ' '
#define NOKEY ERR

// Timing and speed
#define FRAMES_PER_SECOND 20
#define TELEPORTER_INTERVAL_FRAMES (3 * FRAMES_PER_SECOND) // 3 seconds
#define TELEPORTER_WARNING_FRAMES (FRAMES_PER_SECOND / 2)   // 0.5 seconds warning
#define OBSTACLE_ACTIVE_DURATION_FRAMES FRAMES_PER_SECOND  // 1 second

// Color pair identifiers
#define DEFAULT_COLOR 0
#define PLAY_COLOR 1
#define STAT_COLOR 2
#define DEBUG_COLOR 3
#define END_WIN_COLOR 4
#define END_LOSE_COLOR 5
#define ENEMY_COLOR 6
#define TOTEM_COLOR 7
#define OBSTACLE_COLOR 8
#define PLAYER_COLOR 9
#define SPIKE_COLOR 10
#define FIREJET_COLOR 11
#define TELEPORTER_WARNING_COLOR 12
#define CHASER_SLEEPING_COLOR 13
#define CHASER_ANGRY_COLOR 14

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
#define OUT_OF_LIFES 2
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

// Aggressive obstacle type enumeration
typedef enum {
  AGGRESSIVE_SPIKE,
  AGGRESSIVE_FIREJET,
  AGGRESSIVE_TYPE_UNKNOWN
} AGGRESSIVE_OBSTACLE_TYPE;

// Fire jet direction enumeration
typedef enum {
  OBSTACLE_UP,
  OBSTACLE_DOWN,
  OBSTACLE_LEFT,
  OBSTACLE_RIGHT
} AGGRESSIVE_OBSTACLE_DIRECTION;

// Aggressive obstacle structure
typedef struct {
  POSITION pos;
  AGGRESSIVE_OBSTACLE_TYPE type;
  AGGRESSIVE_OBSTACLE_DIRECTION direction;
  int fire_length;     // For fire jets - how far fire travels
  int active_interval; // Frames between activations
  int last_active_frame;
  int is_active;
} AGGRESSIVE_OBSTACLE;

// Enemy type enumeration
typedef enum {
  ENEMY_PATROL_HORIZONTAL,
  ENEMY_PATROL_VERTICAL,
  ENEMY_PATROL_RANDOM,
  ENEMY_CHASER,
  ENEMY_TELEPORTER,
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
  int last_teleport_frame; // For teleporter enemies
  int chase_radius;        // For chaser enemies - chase range
  int is_chasing;          // For chaser enemies - 1 if chasing, 0 if sleeping
} ENEMY;

// Level structure
typedef struct {
  int time_limit;
  int lifes;
  int win_score;
  int width, height;
  POSITION start_pos;
  int num_totems;
  TOTEM *totems;
  int num_obstacles;
  OBSTACLE *obstacles;
  int num_enemies;
  ENEMY *enemies;
  int num_aggressive_obstacles;
  AGGRESSIVE_OBSTACLE *aggressive_obstacles;
} LEVEL;

typedef enum {
  LIFE_STATUS_NONE,
  LIFE_PATROL_KILL,
  LIFE_CHASER_KILL,
  LIFE_TELEPORTER_KILL,
  LIFE_FIRE_JET,
  LIFE_SPIKE_WALK
} LIFE_LOST_STATUS;

// Game state structure
typedef struct {
  int score;
  int time_elapsed;
  int frames_elapsed;
  int lifes_left;
  char debug_msg[MAX_DEBUG_LEN];
  int status;
  LIFE_LOST_STATUS life_loss;
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
