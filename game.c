#include "game.h"
#include "level.h"
#include "window.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

const int FRAME_TIME_MS = 1000 / FRAMES_PER_SECOND;

GAMESTATE *InitGame(void) {
  GAMESTATE *state = (GAMESTATE *)malloc(sizeof(GAMESTATE));

  if (state == NULL) {
    fprintf(stderr, "Error allocating memory for game state\n");
    exit(EXIT_FAILURE);
  }

  // Seed random number generator for enemy movement
  srand((unsigned int)time(NULL));

  state->level = LoadLevel(LEVEL_FILE);

  if (state->level == NULL) {
    free(state);
    fprintf(stderr, "Error loading level file: %s\n", LEVEL_FILE);
    exit(EXIT_FAILURE);
  }

  state->score = 0;
  state->time_elapsed = 0;
  state->frames_elapsed = 0;
  state->lifes_left = state->level->lifes;
  state->life_loss = LIFE_STATUS_NONE;
  state->status = CONTINUE;
  state->player_pos = state->level->start_pos;

  snprintf(state->debug_msg, MAX_DEBUG_LEN, "Game initialized.");

  return state;
}

int CheckWithinBounds(POSITION pos1, POSITION pos2, POSITION point) {
  return (point.x >= pos1.x && point.x <= pos2.x && point.y >= pos1.y &&
          point.y <= pos2.y);
}

int CheckCollision(GAMESTATE *state, POSITION wanted_pos) {
  for (int i = 0; i < state->level->num_obstacles; i++) {
    OBSTACLE *obs = &state->level->obstacles[i];
    if (CheckWithinBounds(obs->pos1, obs->pos2, wanted_pos)) {
      return FALSE;
    }
  }

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

  for (int i = 0; i < state->level->num_aggressive_obstacles; i++) {
    AGGRESSIVE_OBSTACLE *obs = &state->level->aggressive_obstacles[i];
    if (obs->type == AGGRESSIVE_SPIKE && obs->pos.x == wanted_pos.x &&
        obs->pos.y == wanted_pos.y && obs->direction == player_dir) {
      return FALSE;
    }
  }

  return CheckWithinBounds(
      (POSITION){0, 0},
      (POSITION){state->level->width - 1, state->level->height - 1},
      wanted_pos);
}

void CollectTotems(GAMESTATE *state) {
  for (int i = 0; i < state->level->num_totems; i++) {
    TOTEM *totem = &state->level->totems[i];
    if (!totem->is_collected && totem->pos.x == state->player_pos.x &&
        totem->pos.y == state->player_pos.y) {
      state->score += totem->points;
      totem->is_collected = TRUE;
    }
  }
}

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

void HandlePlayerKill(GAMESTATE *state, LIFE_LOST_STATUS reason) {
  state->lifes_left--;
  state->life_loss = reason;
}

void MoveEnemies(GAMESTATE *state) {
  for (int i = 0; i < state->level->num_enemies; i++) {
    ENEMY *enemy = &state->level->enemies[i];
    if (enemy->type == ENEMY_TYPE_UNKNOWN) {
      continue;
    }

    if (state->frames_elapsed % (FRAMES_PER_SECOND / enemy->speed) != 0) {
      continue;
    }

    POSITION wanted_pos = enemy->current_pos;

    // Handle different enemy types
    if (enemy->type == ENEMY_PATROL_HORIZONTAL ||
        enemy->type == ENEMY_PATROL_VERTICAL) {
      wanted_pos = NextEnemyPosition(enemy);

      if (!CheckCollision(state, wanted_pos) ||
          !CheckWithinBounds(enemy->pos1, enemy->pos2, wanted_pos)) {
        enemy->direction *= -1;
        enemy->current_pos = NextEnemyPosition(enemy);
        continue;
      }

      enemy->current_pos = wanted_pos;

    } else if (enemy->type == ENEMY_PATROL_RANDOM) {
      wanted_pos = NextEnemyPosition(enemy);

      if (CheckCollision(state, wanted_pos) &&
          CheckWithinBounds(enemy->pos1, enemy->pos2, wanted_pos)) {
        enemy->current_pos = wanted_pos;
      }
    } else if (enemy->type == ENEMY_CHASER) {
      // Calculate distance to player
      int dx = state->player_pos.x - enemy->current_pos.x;
      int dy = state->player_pos.y - enemy->current_pos.y;
      int distance = (int)sqrt(dx * dx + dy * dy);

      // Only chase if player is within chase radius ("can be seen")
      if (distance <= enemy->chase_radius) {
        // Move towards player (simple AI: one step at a time)
        if (abs(dx) > abs(dy)) {
          wanted_pos.x += (dx > 0) ? 1 : -1;
        } else {
          wanted_pos.y += (dy > 0) ? 1 : -1;
        }

        // Only move if within bounds and no collision
        if (CheckCollision(state, wanted_pos) &&
            CheckWithinBounds(enemy->pos1, enemy->pos2, wanted_pos)) {
          enemy->current_pos = wanted_pos;
        }
      }

    } else if (enemy->type == ENEMY_TELEPORTER) {
      // Teleport every 3 seconds
      if (state->frames_elapsed - enemy->last_teleport_frame >=
          TELEPORTER_INTERVAL_FRAMES) {
        int range_x = enemy->pos2.x - enemy->pos1.x + 1;
        int range_y = enemy->pos2.y - enemy->pos1.y + 1;

        // Random position within bounding box
        wanted_pos.x = enemy->pos1.x + (rand() % range_x);
        wanted_pos.y = enemy->pos1.y + (rand() % range_y);

        if (CheckCollision(state, wanted_pos)) {
          enemy->current_pos = wanted_pos;
          enemy->last_teleport_frame = state->frames_elapsed;
        } else {
          fprintf(stderr,
                  "Warning: Teleporter enemy %d attempted invalid teleport.\n",
                  i);
        }
      }
    }
  }
}

void CheckPlayerEnemyCollision(GAMESTATE *state) {
  for (int i = 0; i < state->level->num_enemies; i++) {
    ENEMY *enemy = &state->level->enemies[i];
    if (enemy->current_pos.x == state->player_pos.x &&
        enemy->current_pos.y == state->player_pos.y) {
      HandlePlayerKill(state,
                       (enemy->type == ENEMY_TELEPORTER) ? LIFE_TELEPORTER_KILL
                       : (enemy->type == ENEMY_CHASER)   ? LIFE_CHASER_KILL
                                                         : LIFE_PATROL_KILL);
      break;
    }
  }
}

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

void UpdateAggressiveObstacles(GAMESTATE *state) {
  for (int i = 0; i < state->level->num_aggressive_obstacles; i++) {
    AGGRESSIVE_OBSTACLE *obs = &state->level->aggressive_obstacles[i];

    int frames_since_last_active =
        state->frames_elapsed - obs->last_active_frame;

    // Fire for 1 second, then wait for interval
    if (frames_since_last_active >= obs->active_interval) {
      obs->is_active = TRUE;

      if (frames_since_last_active >=
          obs->active_interval + OBSTACLE_ACTIVE_DURATION_FRAMES) {
        obs->is_active = FALSE;
        obs->last_active_frame = state->frames_elapsed;
      }
    }

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

void FinishGame(WIN *playwin, GAMESTATE *state) {
  switch (state->status) {
  case GAME_TIMEOUT:
    EndGame(playwin, "Time's up!", END_LOSE_COLOR);
    break;
  case OUT_OF_LIFES:
    EndGame(playwin, "Out of lifes!", END_LOSE_COLOR);
    break;
  case USER_QUIT:
    EndGame(playwin, "You quit the game.", END_LOSE_COLOR);
    break;
  case CONTINUE:
    EndGame(playwin, "Game ended unexpectedly.", END_LOSE_COLOR);
    break;
  case WIN_POINTS:
    EndGame(playwin, "You collected enough points!", END_WIN_COLOR);
    break;
  }

  char end_msg[COLS];
  snprintf(end_msg, sizeof(end_msg), "Your final score: %d", state->score);
  mvwprintw(playwin->window, playwin->rows / 2 + 1,
            (playwin->cols - strlen(end_msg)) / 2, "%s", end_msg);
  wrefresh(playwin->window);
}

void MainLoop(WIN *playwin, WIN *statwin, WIN *debugwin, GAMESTATE *state) {
  int ch;
  int is_paused = FALSE;
  POSITION wanted_pos;

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

    wanted_pos.x = state->player_pos.x;
    wanted_pos.y = state->player_pos.y;

    if (ch == UP) {
      wanted_pos.y++;
    } else if (ch == DOWN) {
      wanted_pos.y--;
    } else if (ch == LEFT) {
      wanted_pos.x--;
    } else if (ch == RIGHT) {
      wanted_pos.x++;
    }

    if (CheckCollision(state, wanted_pos)) {
      state->player_pos = wanted_pos;
    }

    snprintf(state->debug_msg, MAX_DEBUG_LEN, "Player pos: (%d,%d)",
             state->player_pos.x, state->player_pos.y);

    CollectTotems(state);
    MoveEnemies(state);
    CheckPlayerEnemyCollision(state);
    UpdateAggressiveObstacles(state);
    RenderPlayArea(playwin, state);

    state->frames_elapsed++;
    state->time_elapsed = state->frames_elapsed / FRAMES_PER_SECOND;

    ShowDebug(debugwin, state->debug_msg);
    ShowStatus(statwin, state);
    wrefresh(playwin->window);

    if (state->time_elapsed >= state->level->time_limit) {
      state->status = GAME_TIMEOUT;
      break;
    }

    if (state->lifes_left <= 0) {
      state->status = OUT_OF_LIFES;
      break;
    }

    if (state->score >= state->level->win_score) {
      state->status = WIN_POINTS;
      break;
    }

    if (state->life_loss != LIFE_STATUS_NONE) {
      ShowLifeLoss(playwin, state->life_loss);
      state->life_loss = LIFE_STATUS_NONE;
      state->player_pos = state->level->start_pos;
      usleep(2 * 1000 * 1000); // Pause for 2 seconds to show life loss message
    }

    flushinp();

    usleep(FRAME_TIME_MS * 1000);
  }
}
