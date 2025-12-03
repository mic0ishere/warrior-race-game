#include "game.h"
#include "level.h"
#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const int FRAME_TIME_MS = 1000 / FRAMES_PER_SECOND;

GAMESTATE *InitGame(void) {
  GAMESTATE *state = (GAMESTATE *)malloc(sizeof(GAMESTATE));

  if (state == NULL) {
    fprintf(stderr, "Error allocating memory for game state\n");
    exit(EXIT_FAILURE);
  }

  state->level = LoadLevel(LEVEL_FILE);

  if (state->level == NULL) {
    free(state);
    fprintf(stderr, "Error loading level file: %s\n", LEVEL_FILE);
    exit(EXIT_FAILURE);
  }

  state->score = 0;
  state->time_elapsed = 0;
  state->frames_elapsed = 0;
  state->lives_left = state->level->lives;
  state->status = CONTINUE;
  state->player_pos = state->level->start_pos;

  snprintf(state->debug_msg, MAX_DEBUG_LEN, "Game initialized.");

  return state;
}

int CheckWithinBounds(int pos1_x, int pos1_y, int pos2_x, int pos2_y,
                      POSITION point) {
  return (point.x >= pos1_x && point.x <= pos2_x && point.y >= pos1_y &&
          point.y <= pos2_y);
}

int CheckCollision(GAMESTATE *state, POSITION wanted_pos) {
  for (int i = 0; i < state->level->num_obstacles; i++) {
    OBSTACLE *obs = &state->level->obstacles[i];
    if (CheckWithinBounds(obs->pos1.x, obs->pos1.y, obs->pos2.x, obs->pos2.y,
                          wanted_pos)) {
      return FALSE;
    }
  }

  return CheckWithinBounds(0, 0, state->level->width - 1,
                           state->level->height - 1, wanted_pos);
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

POSITION NextEnemyPosition(ENEMY *enemy) {
  POSITION next_pos = enemy->current_pos;
  int move_amount = enemy->direction;

  if (enemy->type == ENEMY_PATROL_HORIZONTAL) {
    next_pos.x += move_amount;
  } else if (enemy->type == ENEMY_PATROL_VERTICAL) {
    next_pos.y += move_amount;
  }

  return next_pos;
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

    POSITION wanted_pos = NextEnemyPosition(enemy);

    if (!CheckCollision(state, wanted_pos) ||
        !CheckWithinBounds(enemy->pos1.x, enemy->pos1.y, enemy->pos2.x,
                           enemy->pos2.y, wanted_pos)) {
      enemy->direction *= -1;
      enemy->current_pos = NextEnemyPosition(enemy);
      continue;
    }

    enemy->current_pos = wanted_pos;
  }
}

void CheckPlayerEnemyCollision(GAMESTATE *state) {
  for (int i = 0; i < state->level->num_enemies; i++) {
    ENEMY *enemy = &state->level->enemies[i];
    if (enemy->current_pos.x == state->player_pos.x &&
        enemy->current_pos.y == state->player_pos.y) {
      state->lives_left--;
      state->player_pos = state->level->start_pos;
      break;
    }
  }
}

void FinishGame(WIN *playwin, GAMESTATE *state) {
  switch (state->status) {
  case GAME_TIMEOUT:
    EndGame(playwin, "Time's up!", END_LOSE_COLOR);
    break;
  case OUT_OF_LIVES:
    EndGame(playwin, "Out of lives!", END_LOSE_COLOR);
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
  POSITION wanted_pos;

  while (1) {
    ch = wgetch(statwin->window);

    if (ch == QUIT) {
      state->status = USER_QUIT;
      break;
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

    RenderPlayArea(playwin, state);
    CollectTotems(state);
    MoveEnemies(state);
    CheckPlayerEnemyCollision(state);

    state->frames_elapsed++;
    state->time_elapsed = state->frames_elapsed / FRAMES_PER_SECOND;

    ShowDebug(debugwin, state->debug_msg);
    ShowStatus(statwin, state);
    wrefresh(playwin->window);

    if (state->time_elapsed >= state->level->time_limit) {
      state->status = GAME_TIMEOUT;
      break;
    }

    if (state->lives_left <= 0) {
      state->status = OUT_OF_LIVES;
      break;
    }

    if (state->score >= state->level->win_score) {
      state->status = WIN_POINTS;
      break;
    }

    flushinp();

    usleep(FRAME_TIME_MS * 1000);
  }
}
