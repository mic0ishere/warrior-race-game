#include "game.h"
#include "menu.h"
#include "ranking.h"
#include "window.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

const int FRAME_TIME_MS = 1000 / FRAMES_PER_SECOND;

// Initialize game state with default values for a new game session
GAMESTATE *InitGame(LEVEL *level) {
  GAMESTATE *state = (GAMESTATE *)malloc(sizeof(GAMESTATE));

  if (state == NULL) {
    fprintf(stderr, "Error allocating memory for game state\n");
    free(level);
    exit(EXIT_FAILURE);
  }

  // Seed random number generator for enemy movement and behavior
  srand((unsigned int)time(NULL));

  state->level = level;
  state->score = 0;
  state->time_elapsed = 0;
  state->frames_elapsed = 0;
  state->lifes_left = state->level->lifes;
  state->life_loss = LIFE_STATUS_NONE;
  state->status = CONTINUE;
  state->player_pos = state->level->start_pos;
  state->num_active_powerups = 0;
  state->has_shield = FALSE;
  state->shield_protection_timer = 0;
  state->extra_life_timer = 0;
  state->is_in_tall_grass = FALSE;
  state->sticky_floor_counter = 0;
  state->last_teleport_frame = -TELEPORT_PAD_COOLDOWN_FRAMES;
  state->grass_vision_transition = 0;

  // Initialize all power-up slots as inactive
  for (int i = 0; i < MAX_ACTIVE_POWERUPS; i++) {
    state->active_powerups[i].is_active = FALSE;
    state->active_powerups[i].frames_remaining = 0;
    state->active_powerups[i].type = POWERUP_TYPE_UNKNOWN;
  }

  snprintf(state->debug_msg, MAX_DEBUG_LEN, "Game initialized.");

  return state;
}

// Check if a point is within the bounds defined by two positions
int CheckWithinBounds(POSITION pos1, POSITION pos2, POSITION point) {
  return (point.x >= pos1.x && point.x <= pos2.x && point.y >= pos1.y &&
          point.y <= pos2.y);
}

// Check if player movement to wanted_pos would result in collision
// Returns FALSE if collision detected (blocking movement), TRUE if valid
int CheckCollision(GAMESTATE *state, POSITION wanted_pos) {
  // Check collision with static rectangular obstacles
  for (int i = 0; i < state->level->num_obstacles; i++) {
    OBSTACLE *obs = &state->level->obstacles[i];
    if (CheckWithinBounds(obs->pos1, obs->pos2, wanted_pos)) {
      return FALSE;
    }
  }

  // Determine player's movement direction for spike collision detection
  POSITION previous_pos = state->player_pos;
  AGGRESSIVE_OBSTACLE_DIRECTION player_dir;
  if (wanted_pos.x > previous_pos.x) {
    player_dir = OBSTACLE_RIGHT;
  } else if (wanted_pos.x < previous_pos.x) {
    player_dir = OBSTACLE_LEFT;
  } else if (wanted_pos.y < previous_pos.y) {
    player_dir = OBSTACLE_DOWN;
  } else {
    player_dir = OBSTACLE_UP;
  }

  // Check collision with aggressive obstacles (spikes, firejets)
  for (int i = 0; i < state->level->num_aggressive_obstacles; i++) {
    AGGRESSIVE_OBSTACLE *obs = &state->level->aggressive_obstacles[i];
    // Spikes only block movement when player moves INTO the spike direction
    if (obs->type == AGGRESSIVE_SPIKE && obs->pos.x == wanted_pos.x &&
        obs->pos.y == wanted_pos.y && obs->direction == player_dir) {
      return FALSE;
    }

    // Firejet source positions are solid obstacles (can't walk through)
    if (obs->type == AGGRESSIVE_FIREJET && obs->pos.x == wanted_pos.x &&
        obs->pos.y == wanted_pos.y) {
      return FALSE;
    }
  }

  // Check if position is within level boundaries
  return CheckWithinBounds(
      (POSITION){0, 0},
      (POSITION){state->level->width - 1, state->level->height - 1},
      wanted_pos);
}

// Check for totems at player position and collect them (with double points if active)
void CollectTotems(GAMESTATE *state) {
  for (int i = 0; i < state->level->num_totems; i++) {
    TOTEM *totem = &state->level->totems[i];
    if (!totem->is_collected && totem->pos.x == state->player_pos.x &&
        totem->pos.y == state->player_pos.y) {
      int points = totem->points;
      if (IsPowerupActive(state, POWERUP_DOUBLE_POINTS)) {
        points *= 2;
      }
      state->score += points;
      totem->is_collected = TRUE;
    }
  }
}

// Check if a specific power-up type is currently active for the player
int IsPowerupActive(GAMESTATE *state, POWERUP_TYPE type) {
  for (int i = 0; i < state->num_active_powerups; i++) {
    if (state->active_powerups[i].is_active &&
        state->active_powerups[i].type == type) {
      return TRUE;
    }
  }
  return FALSE;
}

// Activate a timed power-up (double points, time freeze) with specified duration
int ActivateTimedPowerup(GAMESTATE *state, POWERUP_TYPE type,
                         int duration_frames) {
  if (state->num_active_powerups < MAX_ACTIVE_POWERUPS) {
    int slot = state->num_active_powerups;
    state->active_powerups[slot].type = type;
    state->active_powerups[slot].frames_remaining = duration_frames;
    state->active_powerups[slot].is_active = TRUE;
    state->num_active_powerups++;
    return TRUE;
  }
  return FALSE;
}

// Check for power-ups at player position and activate their effects
void CollectPowerups(GAMESTATE *state) {
  for (int i = 0; i < state->level->num_powerups; i++) {
    POWERUP *powerup = &state->level->powerups[i];
    if (!powerup->is_collected && powerup->pos.x == state->player_pos.x &&
        powerup->pos.y == state->player_pos.y) {
      powerup->is_collected = TRUE;

      switch (powerup->type) {
      case POWERUP_SHIELD:
        state->has_shield = TRUE; // Protects from one enemy hit
        break;
      case POWERUP_EXTRA_LIFE:
        state->lifes_left++;
        state->extra_life_timer = 2 * FRAMES_PER_SECOND; // 2 seconds display
        break;
      case POWERUP_DOUBLE_POINTS:
        ActivateTimedPowerup(state, POWERUP_DOUBLE_POINTS,
                             DOUBLE_POINTS_DURATION_FRAMES);
        break;
      case POWERUP_TIME_FREEZE:
        ActivateTimedPowerup(state, POWERUP_TIME_FREEZE,
                             TIME_FREEZE_DURATION_FRAMES);
        break;
      default:
        break;
      }
    }
  }
}

// Decrement remaining frames for active power-ups and remove expired ones
void UpdateActivePowerups(GAMESTATE *state) {
  int i = 0;
  while (i < state->num_active_powerups) {
    if (state->active_powerups[i].is_active) {
      state->active_powerups[i].frames_remaining--;

      if (state->active_powerups[i].frames_remaining <= 0) {
        state->active_powerups[i].is_active = FALSE;

        // Shift remaining power-ups down in array to fill gap
        for (int j = i; j < state->num_active_powerups - 1; j++) {
          state->active_powerups[j] = state->active_powerups[j + 1];
        }
        state->num_active_powerups--;
        continue;
      }
    }
    i++;
  }
}

// Move a position in the specified direction by amount cells
void MoveInDirection(POSITION *pos, int direction, int amount) {
  switch (direction) {
  case OBSTACLE_DOWN:
    pos->y -= amount;
    break;
  case OBSTACLE_UP:
    pos->y += amount;
    break;
  case OBSTACLE_LEFT:
    pos->x -= amount;
    break;
  case OBSTACLE_RIGHT:
    pos->x += amount;
    break;
  }
}

// Calculate next position for enemy based on its type and current state
POSITION NextEnemyPosition(ENEMY *enemy) {
  POSITION next_pos = enemy->current_pos;
  int move_amount = enemy->direction;

  if (enemy->type == ENEMY_PATROL_HORIZONTAL) {
    next_pos.x += move_amount;
  } else if (enemy->type == ENEMY_PATROL_VERTICAL) {
    next_pos.y += move_amount;
  } else if (enemy->type == ENEMY_PATROL_RANDOM) {
    // Random movement: choose a random direction
    int rand_dir = rand() % 4;
    MoveInDirection(&next_pos, rand_dir, 1);
  }

  return next_pos;
}

// Handle player death - decrement lives and set death reason
void HandlePlayerKill(GAMESTATE *state, LIFE_LOST_STATUS reason) {
  state->lifes_left--;
  state->life_loss = reason;
}

// Move patrol-type enemy (horizontal/vertical) with bounce on boundaries
static void MovePatrolEnemy(GAMESTATE *state, ENEMY *enemy) {
  POSITION wanted_pos = NextEnemyPosition(enemy);

  // Bounce off obstacles or boundaries by reversing direction
  if (!CheckCollision(state, wanted_pos) ||
      !CheckWithinBounds(enemy->pos1, enemy->pos2, wanted_pos)) {
    enemy->direction *= -1;
    enemy->current_pos = NextEnemyPosition(enemy);
    return;
  }

  enemy->current_pos = wanted_pos;
}

// Move random walker enemy - only moves if destination is valid
static void MoveRandomEnemy(GAMESTATE *state, ENEMY *enemy) {
  POSITION wanted_pos = NextEnemyPosition(enemy);

  if (CheckCollision(state, wanted_pos) &&
      CheckWithinBounds(enemy->pos1, enemy->pos2, wanted_pos)) {
    enemy->current_pos = wanted_pos;
  }
}

// Move chaser enemy - pursues player when within chase radius
static void MoveChaserEnemy(GAMESTATE *state, ENEMY *enemy) {
  int dx = state->player_pos.x - enemy->current_pos.x;
  int dy = state->player_pos.y - enemy->current_pos.y;
  int distance = (int)sqrt(dx * dx + dy * dy); // Calculate distance to player

  if (distance <= enemy->chase_radius) {
    enemy->is_chasing = TRUE;
    POSITION wanted_pos = enemy->current_pos;

    // Move toward player on axis with greater distance
    if (abs(dx) > abs(dy)) {
      wanted_pos.x += (dx > 0) ? 1 : -1;
    } else {
      wanted_pos.y += (dy > 0) ? 1 : -1;
    }

    if (CheckCollision(state, wanted_pos) &&
        CheckWithinBounds(enemy->pos1, enemy->pos2, wanted_pos)) {
      enemy->current_pos = wanted_pos;
    }
  } else {
    enemy->is_chasing = FALSE; // Too far, stop chasing
  }
}

// Move teleporter enemy - teleports to random location within bounds at intervals
// Move teleporter enemy - teleports to random location within bounds at intervals
static void MoveTeleporterEnemy(GAMESTATE *state, ENEMY *enemy) {
  // Check if enough time has passed since last teleport (speed affects interval)
  if (state->frames_elapsed - enemy->last_teleport_frame <
      (TELEPORTER_INTERVAL_FRAMES / enemy->speed)) {
    return;
  }

  // Calculate teleport range
  int range_x = enemy->pos2.x - enemy->pos1.x + 1;
  int range_y = enemy->pos2.y - enemy->pos1.y + 1;

  // Choose random position within bounds
  POSITION wanted_pos;
  wanted_pos.x = enemy->pos1.x + (rand() % range_x);
  wanted_pos.y = enemy->pos1.y + (rand() % range_y);

  int is_colliding = !CheckCollision(state, wanted_pos);
  int is_at_start = (wanted_pos.x == state->level->start_pos.x &&
                     wanted_pos.y == state->level->start_pos.y);

  // Don't teleport onto player start position or collision
  if (!is_at_start && !is_colliding) {
    enemy->current_pos = wanted_pos;
    enemy->last_teleport_frame = state->frames_elapsed;
  }
}

// Update all enemy positions based on their type and speed
void MoveEnemies(GAMESTATE *state) {
  // Time freeze power-up stops all enemy movement
  if (IsPowerupActive(state, POWERUP_TIME_FREEZE)) {
    return;
  }

  for (int i = 0; i < state->level->num_enemies; i++) {
    ENEMY *enemy = &state->level->enemies[i];
    if (enemy->type == ENEMY_TYPE_UNKNOWN) {
      continue;
    }

    // Speed-based frame throttling - faster enemies move more often
    if (state->frames_elapsed % (FRAMES_PER_SECOND / enemy->speed) != 0) {
      continue;
    }

    // Move enemy based on type
    if (enemy->type == ENEMY_PATROL_HORIZONTAL ||
        enemy->type == ENEMY_PATROL_VERTICAL) {
      MovePatrolEnemy(state, enemy);
    } else if (enemy->type == ENEMY_PATROL_RANDOM) {
      MoveRandomEnemy(state, enemy);
    } else if (enemy->type == ENEMY_CHASER) {
      MoveChaserEnemy(state, enemy);
    } else if (enemy->type == ENEMY_TELEPORTER) {
      MoveTeleporterEnemy(state, enemy);
    }
  }
}

// Check if player collided with any enemy and handle shield/death
void CheckPlayerEnemyCollision(GAMESTATE *state) {
  // If shield protection timer is active, player is immune
  if (state->shield_protection_timer > 0) {
    return;
  }

  for (int i = 0; i < state->level->num_enemies; i++) {
    ENEMY *enemy = &state->level->enemies[i];
    if (enemy->current_pos.x == state->player_pos.x &&
        enemy->current_pos.y == state->player_pos.y) {
      // Shield absorbs one hit and grants temporary immunity
      if (state->has_shield) {
        state->has_shield = FALSE;
        state->shield_protection_timer =
            FRAMES_PER_SECOND; // 1 second of protection
        break;
      }
      // No shield - kill player with type-specific death message
      HandlePlayerKill(state,
                       (enemy->type == ENEMY_TELEPORTER) ? LIFE_TELEPORTER_KILL
                       : (enemy->type == ENEMY_CHASER)   ? LIFE_CHASER_KILL
                                                         : LIFE_PATROL_KILL);
      break;
    }
  }
}

// Check if player is standing on an active fire path from a firejet
int IsOnFirePath(AGGRESSIVE_OBSTACLE *firejet, POSITION player_pos) {
  if (!firejet->is_active) {
    return FALSE;
  }

  // Check if player is on the fire path
  for (int i = 1; i <= firejet->fire_length; i++) {
    POSITION fire_pos = firejet->pos;
    MoveInDirection(&fire_pos, firejet->direction, i);

    if (fire_pos.x == player_pos.x && fire_pos.y == player_pos.y) {
      return TRUE;
    }
  }

  return FALSE;
}

// Update aggressive obstacle states and check for player damage
void UpdateAggressiveObstacles(GAMESTATE *state) {
  for (int i = 0; i < state->level->num_aggressive_obstacles; i++) {
    AGGRESSIVE_OBSTACLE *obs = &state->level->aggressive_obstacles[i];

    int frames_since_last_active =
        state->frames_elapsed - obs->last_active_frame;

    // Activate obstacle for 1 second, then wait for interval before next activation
    if (frames_since_last_active >= obs->active_interval) {
      obs->is_active = TRUE;

      if (frames_since_last_active >=
          obs->active_interval + OBSTACLE_ACTIVE_DURATION_FRAMES) {
        obs->is_active = FALSE;
        obs->last_active_frame = state->frames_elapsed;
      }
    }

    // Check if player is hit by active obstacles
    if (obs->type == AGGRESSIVE_SPIKE) {
      if (obs->pos.x == state->player_pos.x &&
          obs->pos.y == state->player_pos.y && obs->is_active) {
        HandlePlayerKill(state, LIFE_SPIKE_WALK);
        break;
      }
    } else if (obs->type == AGGRESSIVE_FIREJET) {
      if (IsOnFirePath(obs, state->player_pos)) {
        HandlePlayerKill(state, LIFE_FIRE_JET);
        break;
      }
    }
  }
}

// Attempt to teleport player to linked teleport pad (with cooldown check)
static int AttemptPlayerTeleport(GAMESTATE *state, PASSIVE_OBSTACLE *obs) {
  int cooldown = state->frames_elapsed - state->last_teleport_frame;
  if (cooldown < TELEPORT_PAD_COOLDOWN_FRAMES) {
    return FALSE;
  }

  for (int j = 0; j < state->level->num_passive_obstacles; j++) {
    PASSIVE_OBSTACLE *target = &state->level->passive_obstacles[j];

    if (target->type == PASSIVE_TELEPORT_PAD &&
        target->teleport_id == obs->teleport_target_id) {
      if (CheckCollision(state, target->pos1)) {
        state->player_pos = target->pos1;
        state->last_teleport_frame = state->frames_elapsed;
        snprintf(state->debug_msg, MAX_DEBUG_LEN, "Teleported!");
        return TRUE;
      }
      break;
    }
  }
  return FALSE;
}

// Update grass vision transition effect for smooth visibility changes
static void UpdateGrassVisionTransition(GAMESTATE *state, int was_in_grass) {
  if (state->is_in_tall_grass && !was_in_grass) {
    state->grass_vision_transition = 0;
  } else if (!state->is_in_tall_grass && was_in_grass) {
    state->grass_vision_transition = TALL_GRASS_TRANSITION_FRAMES;
  }

  if (state->is_in_tall_grass &&
      state->grass_vision_transition < TALL_GRASS_TRANSITION_FRAMES) {
    state->grass_vision_transition++;
  } else if (!state->is_in_tall_grass && state->grass_vision_transition > 0) {
    state->grass_vision_transition--;
  }
}

// Check and handle passive obstacle effects (teleport pads, sticky floor, tall grass)
void CheckPassiveObstacles(GAMESTATE *state) {
  int was_in_grass = state->is_in_tall_grass;
  state->is_in_tall_grass = FALSE;
  int is_on_sticky = FALSE;

  for (int i = 0; i < state->level->num_passive_obstacles; i++) {
    PASSIVE_OBSTACLE *obs = &state->level->passive_obstacles[i];

    if (!CheckWithinBounds(obs->pos1, obs->pos2, state->player_pos)) {
      continue;
    }

    if (obs->type == PASSIVE_TELEPORT_PAD) {
      if (AttemptPlayerTeleport(state, obs)) {
        return;
      }
    } else if (obs->type == PASSIVE_STICKY_FLOOR) {
      is_on_sticky = TRUE;
    } else if (obs->type == PASSIVE_TALL_GRASS) {
      state->is_in_tall_grass = TRUE;
    }
  }

  UpdateGrassVisionTransition(state, was_in_grass);

  if (!is_on_sticky) {
    state->sticky_floor_counter = 0;
  }
}

// Display end-game message and save ranking (if not user quit)
void FinishGame(WIN *playwin, GAMESTATE *state) {
  switch (state->status) {
  case GAME_TIMEOUT:
    EndGame(playwin, "Time's up!", FALSE);
    break;
  case OUT_OF_LIFES:
    EndGame(playwin, "Out of lifes!", FALSE);
    break;
  case USER_QUIT:
    EndGame(playwin, "You quit the game.", FALSE);
    break;
  case CONTINUE:
    EndGame(playwin, "Game ended unexpectedly.", FALSE);
    break;
  case WIN_POINTS:
    EndGame(playwin, "You collected enough points!", TRUE);
    break;
  }

  // Don't display final score calculation if user quit
  if (state->status == USER_QUIT) {
    return;
  }

  SaveRanking(state->level->level_name, state);

  int time_left = MAX(0, state->level->time_limit - state->time_elapsed);

  char score_msg[COLS];
  snprintf(score_msg, sizeof(score_msg),
           "Totems: %d | Lives: %d x %d | Time: %d x %d", state->score,
           state->lifes_left, FINAL_SCORE_LIFE_BONUS, time_left,
           FINAL_SCORE_TIME_BONUS);
  mvwprintw(playwin->window, playwin->rows / 2 + 1,
            (playwin->cols - strlen(score_msg)) / 2, "%s", score_msg);

  int final_score = CalculateFinalScore(state);

  char final_msg[COLS];
  snprintf(final_msg, sizeof(final_msg), "Final Score: %d", final_score);
  mvwprintw(playwin->window, playwin->rows / 2 + 2,
            (playwin->cols - strlen(final_msg)) / 2, "%s", final_msg);

  wrefresh(playwin->window);
}

// Update game state each frame (timers, enemies, obstacles, passive effects)
static void UpdateGameState(GAMESTATE *state) {
  if (state->extra_life_timer > 0) {
    state->extra_life_timer--;
  }

  if (state->shield_protection_timer > 0) {
    state->shield_protection_timer--;
  }

  MoveEnemies(state);
  CheckPlayerEnemyCollision(state);
  UpdateAggressiveObstacles(state);
  CheckPassiveObstacles(state);

  state->frames_elapsed++;
  state->time_elapsed = state->frames_elapsed / FRAMES_PER_SECOND;
}

// Check if game should end (timeout, out of lives, or all totems collected)
static int CheckForGameEnd(GAMESTATE *state) {
  if (state->time_elapsed >= state->level->time_limit) {
    state->status = GAME_TIMEOUT;
    return TRUE;
  }

  if (state->lifes_left <= 0) {
    state->status = OUT_OF_LIFES;
    return TRUE;
  }

  int all_totems_collected = TRUE;
  for (int i = 0; i < state->level->num_totems; i++) {
    if (!state->level->totems[i].is_collected) {
      all_totems_collected = FALSE;
      break;
    }
  }
  
  if (all_totems_collected) {
    state->status = WIN_POINTS;
    return TRUE;
  }

  return FALSE;
}

// Process player input and handle movement with sticky floor slowdown
static void ProcessPlayerMovement(GAMESTATE *state, int ch) {
  POSITION wanted_pos = state->player_pos;
  int movement_attempted = FALSE;

  if (ch == UP) {
    wanted_pos.y++;
    movement_attempted = TRUE;
  } else if (ch == DOWN) {
    wanted_pos.y--;
    movement_attempted = TRUE;
  } else if (ch == LEFT) {
    wanted_pos.x--;
    movement_attempted = TRUE;
  } else if (ch == RIGHT) {
    wanted_pos.x++;
    movement_attempted = TRUE;
  }

  int is_on_sticky = FALSE;
  for (int i = 0; i < state->level->num_passive_obstacles; i++) {
    PASSIVE_OBSTACLE *obs = &state->level->passive_obstacles[i];
    if (obs->type == PASSIVE_STICKY_FLOOR &&
        CheckWithinBounds(obs->pos1, obs->pos2, state->player_pos)) {
      is_on_sticky = TRUE;
      break;
    }
  }

  // Check if player is on sticky floor for movement slowdown
  if (movement_attempted && is_on_sticky) {
    state->sticky_floor_counter++;
    if (state->sticky_floor_counter < STICKY_FLOOR_SLOWDOWN) {
      movement_attempted = FALSE; // Skip movement - sticky floor slows player
    } else {
      state->sticky_floor_counter = 0;
    }
  }

  if (movement_attempted && CheckCollision(state, wanted_pos)) {
    state->player_pos = wanted_pos;
    // Remove shield immunity after player moves
    if (state->shield_protection_timer > 0) {
      state->shield_protection_timer = 0;
    }
  }
}

// Update and render one frame - collect items, update state, render everything
static void UpdateAndRenderFrame(WIN *playwin, WIN *statwin, WIN *debugwin,
                                 GAMESTATE *state) {
  snprintf(state->debug_msg, MAX_DEBUG_LEN, "Player pos: (%d,%d)",
           state->player_pos.x, state->player_pos.y);

  CollectTotems(state);
  CollectPowerups(state);
  UpdateActivePowerups(state);
  UpdateGameState(state);
  RenderPlayArea(playwin, state);

  ShowDebug(debugwin, state->debug_msg);
  ShowStatus(statwin, state);
  wrefresh(playwin->window);
}

// Display life loss message and respawn player at start position
static void HandleLifeLoss(WIN *playwin, GAMESTATE *state) {
  if (state->life_loss != LIFE_STATUS_NONE) {
    ShowLifeLoss(playwin, state->life_loss);
    state->life_loss = LIFE_STATUS_NONE;
    state->player_pos = state->level->start_pos;
    usleep(2 * 1000 * 1000); // 2 second delay after respawn
  }
}

// Main game loop - handles input, updates game state, renders at 20 FPS
void MainLoop(WIN *playwin, WIN *statwin, WIN *debugwin, GAMESTATE *state) {
  int ch;
  int is_paused = FALSE;

  while (1) {
    ch = wgetch(statwin->window);

    if (ch == QUIT) {
      state->status = USER_QUIT;
      break;
    }

    if (ch == PAUSE) {
      is_paused = !is_paused;
      continue;
    }

    if (is_paused) {
      ShowAlert(playwin, "Game is paused. Press 'P' to resume.");
      usleep(FRAME_TIME_MS * 1000);
      continue;
    }

    ProcessPlayerMovement(state, ch);
    UpdateAndRenderFrame(playwin, statwin, debugwin, state);

    if (CheckForGameEnd(state)) {
      break;
    }

    HandleLifeLoss(playwin, state);

    flushinp();
    usleep(FRAME_TIME_MS * 1000);
  }
}
