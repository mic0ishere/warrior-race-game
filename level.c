#include "level.h"
#include "game.h"
#include "menu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *AddLevelEntity(void *array, size_t entry_size, int *num_entries,
                     const void *new_entry) {
  void *resized_array = realloc(array, (*num_entries + 1) * entry_size);

  if (resized_array == NULL) {
    return NULL;
  }

  memcpy((char *)resized_array + (*num_entries) * entry_size, new_entry,
         entry_size);
  (*num_entries)++;

  return resized_array;
}

void FreeLevel(LEVEL *level) {
  if (level != NULL) {
    free(level->totems);
    free(level->obstacles);
    free(level->enemies);
    free(level->aggressive_obstacles);
    free(level->powerups);
    free(level);
  }
}

ENEMY LoadEnemy(const char *line, int enemy_id) {
  ENEMY enemy;
  char type_str[32] = "";
  POSITION start_pos = {0, 0};

  // Initialize new fields
  enemy.last_teleport_frame = 0;
  enemy.chase_radius = 10; // Default chase radius
  enemy.is_chasing = FALSE;
  enemy.direction = rand() % 2 == 0 ? DIR_FORWARD : DIR_BACKWARD;

  sscanf(line, "%31s (%d,%d) (%d,%d) (%d,%d) %d %d", type_str, &enemy.pos1.x,
         &enemy.pos1.y, &enemy.pos2.x, &enemy.pos2.y, &start_pos.x,
         &start_pos.y, &enemy.speed, &enemy.chase_radius);

  // Validate and clamp speed
  int verified_speed = MIN(MAX(enemy.speed, 1), 5);
  if (verified_speed != enemy.speed) {
    fprintf(stderr,
            "Warning: Enemies[%d] speed %d out of bounds. Clamped to %d.\n",
            enemy_id, enemy.speed, verified_speed);
    enemy.speed = verified_speed;
  }

  // Set starting position
  if (CheckWithinBounds(enemy.pos1, enemy.pos2, start_pos)) {
    enemy.current_pos = start_pos;
  } else {
    enemy.current_pos = enemy.pos1;
  }

  // Determine enemy type
  if (strcmp(type_str, "RANDOM") == 0) {
    enemy.type = ENEMY_PATROL_RANDOM;
  } else if (strcmp(type_str, "CHASER") == 0) {
    enemy.type = ENEMY_CHASER;
  } else if (strcmp(type_str, "TELEPORTER") == 0) {
    int range_x = enemy.pos2.x - enemy.pos1.x + 1;
    int range_y = enemy.pos2.y - enemy.pos1.y + 1;

    if (range_x > 0 && range_y > 0) {
      enemy.type = ENEMY_TELEPORTER;
    } else {
      fprintf(stderr,
              "Warning: Teleporter enemy %d with invalid teleport bounds.\n",
              enemy_id);
      enemy.type = ENEMY_TYPE_UNKNOWN;
    }
  } else if (strcmp(type_str, "PATROL") == 0) {
    if (enemy.pos1.y == enemy.pos2.y) {
      enemy.type = ENEMY_PATROL_HORIZONTAL;
    } else if (enemy.pos1.x == enemy.pos2.x) {
      enemy.type = ENEMY_PATROL_VERTICAL;
    } else {
      enemy.type = ENEMY_TYPE_UNKNOWN;
      fprintf(stderr, "Warning: Patrol enemy %d with invalid patrol bounds.\n",
              enemy_id);
    }
  } else {
    enemy.type = ENEMY_TYPE_UNKNOWN;
    fprintf(stderr, "Warning: Enemies[%d] with unknown patrol type.\n",
            enemy_id);
  }

  return enemy;
}

AGGRESSIVE_OBSTACLE LoadAggressiveObstacle(const char *line, int obs_id) {
  AGGRESSIVE_OBSTACLE obs;
  char type_str[32] = "";
  char dir_str[16] = "";
  float interval = 0.0;

  // Initialize new fields, even if not used for all obstacle types
  obs.direction = OBSTACLE_UP;
  obs.last_active_frame = 0;
  obs.is_active = 0;
  obs.fire_length = 0;

  sscanf(line, "%31s (%d,%d) %15s %f %d", type_str, &obs.pos.x, &obs.pos.y,
         dir_str, &interval, &obs.fire_length);

  obs.active_interval = (int)(interval * FRAMES_PER_SECOND);

  if (strcmp(dir_str, "UP") == 0) {
    obs.direction = OBSTACLE_UP;
  } else if (strcmp(dir_str, "DOWN") == 0) {
    obs.direction = OBSTACLE_DOWN;
  } else if (strcmp(dir_str, "LEFT") == 0) {
    obs.direction = OBSTACLE_LEFT;
  } else if (strcmp(dir_str, "RIGHT") == 0) {
    obs.direction = OBSTACLE_RIGHT;
  }

  if (strcmp(type_str, "SPIKE") == 0) {
    obs.type = AGGRESSIVE_SPIKE;
  } else if (strcmp(type_str, "FIREJET") == 0) {
    obs.type = AGGRESSIVE_FIREJET;
  } else {
    obs.type = AGGRESSIVE_TYPE_UNKNOWN;
    fprintf(stderr, "Warning: AggressiveObstacles[%d] with unknown type.\n",
            obs_id);
  }

  return obs;
}

POWERUP LoadPowerup(const char *line, int powerup_id) {
  POWERUP powerup;
  char type_str[32] = "";

  powerup.is_collected = FALSE;
  sscanf(line, "%31s (%d,%d)", type_str, &powerup.pos.x, &powerup.pos.y);

  if (strcmp(type_str, "SHIELD") == 0) {
    powerup.type = POWERUP_SHIELD;
  } else if (strcmp(type_str, "DOUBLE") == 0) {
    powerup.type = POWERUP_DOUBLE_POINTS;
  } else if (strcmp(type_str, "LIFE") == 0) {
    powerup.type = POWERUP_EXTRA_LIFE;
  } else if (strcmp(type_str, "FREEZE") == 0) {
    powerup.type = POWERUP_TIME_FREEZE;
  } else {
    powerup.type = POWERUP_TYPE_UNKNOWN;
    fprintf(stderr, "Warning: Powerups[%d] with unknown type.\n", powerup_id);
  }

  return powerup;
}

LEVEL *LoadLevel(const char *filename) {
  LEVEL *level = (LEVEL *)malloc(sizeof(LEVEL));

  if (level == NULL) {
    return NULL;
  }

  // Extract level name from filename
  char *level_name = GetLevelName(filename);
  strncpy(level->level_name, level_name, MAX_LEVEL_NAME_LEN - 1);
  level->level_name[MAX_LEVEL_NAME_LEN - 1] = '\0';

  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    free(level);
    return NULL;
  }

  level->totems = NULL;
  level->obstacles = NULL;
  level->enemies = NULL;
  level->aggressive_obstacles = NULL;
  level->powerups = NULL;
  level->num_totems = 0;
  level->num_obstacles = 0;
  level->num_enemies = 0;
  level->num_aggressive_obstacles = 0;
  level->num_powerups = 0;

  char line[MAX_LINE_LEN];
  char *current_section = NULL;

  while (fgets(line, MAX_LINE_LEN, file)) {
    line[strcspn(line, "\n")] = 0;
    if (strlen(line) == 0 || line[0] == '#') {
      continue;
    }

    if (line[0] == '[' && line[strlen(line) - 1] == ']') {
      free(current_section);
      current_section = strdup(line);
      if (current_section == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for section name\n");
        FreeLevel(level);
        fclose(file);
        return NULL;
      }
      continue;
    }

    if (current_section == NULL) {
      continue;
    }

    if (strcmp(current_section, "[CONFIG]") == 0) {
      sscanf(line, "TIME=%d", &level->time_limit);
      sscanf(line, "LIFES=%d", &level->lifes);
      sscanf(line, "WIDTH=%d", &level->width);
      sscanf(line, "HEIGHT=%d", &level->height);
      sscanf(line, "START_X=%d", &level->start_pos.x);
      sscanf(line, "START_Y=%d", &level->start_pos.y);
      sscanf(line, "WIN_SCORE=%d", &level->win_score);
    } else if (strcmp(current_section, "[TOTEMS]") == 0) {
      TOTEM new_totem;
      new_totem.is_collected = FALSE;
      sscanf(line, "(%d,%d) %d", &new_totem.pos.x, &new_totem.pos.y,
             &new_totem.points);

      level->totems = (TOTEM *)AddLevelEntity(level->totems, sizeof(TOTEM),
                                              &level->num_totems, &new_totem);
      if (level->totems == NULL) {
        free(current_section);
        FreeLevel(level);
        fclose(file);
        return NULL;
      }

    } else if (strcmp(current_section, "[OBSTACLES]") == 0) {
      OBSTACLE new_obstacle;
      sscanf(line, "(%d,%d) (%d,%d)", &new_obstacle.pos1.x,
             &new_obstacle.pos1.y, &new_obstacle.pos2.x, &new_obstacle.pos2.y);

      level->obstacles =
          (OBSTACLE *)AddLevelEntity(level->obstacles, sizeof(OBSTACLE),
                                     &level->num_obstacles, &new_obstacle);
      if (level->obstacles == NULL) {
        free(current_section);
        FreeLevel(level);
        fclose(file);
        return NULL;
      }
    } else if (strcmp(current_section, "[ENEMIES]") == 0) {
      ENEMY enemy = LoadEnemy(line, level->num_enemies);
      level->enemies = (ENEMY *)AddLevelEntity(level->enemies, sizeof(ENEMY),
                                               &level->num_enemies, &enemy);
      if (level->enemies == NULL) {
        free(current_section);
        FreeLevel(level);
        fclose(file);
        return NULL;
      }
    } else if (strcmp(current_section, "[AGGRESSIVE_OBSTACLES]") == 0) {
      AGGRESSIVE_OBSTACLE obs =
          LoadAggressiveObstacle(line, level->num_aggressive_obstacles);

      level->aggressive_obstacles = (AGGRESSIVE_OBSTACLE *)AddLevelEntity(
          level->aggressive_obstacles, sizeof(AGGRESSIVE_OBSTACLE),
          &level->num_aggressive_obstacles, &obs);

      if (level->aggressive_obstacles == NULL) {
        free(current_section);
        FreeLevel(level);
        fclose(file);
        return NULL;
      }
    } else if (strcmp(current_section, "[POWERUPS]") == 0) {
      POWERUP powerup = LoadPowerup(line, level->num_powerups);
      level->powerups = (POWERUP *)AddLevelEntity(
          level->powerups, sizeof(POWERUP), &level->num_powerups, &powerup);
      if (level->powerups == NULL) {
        free(current_section);
        FreeLevel(level);
        fclose(file);
        return NULL;
      }
    }
  }

  free(current_section);
  fclose(file);
  return level;
}
