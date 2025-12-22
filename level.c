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
    free(level->passive_obstacles);
    free(level);
  }
}

ENEMY LoadEnemy(const char *line, int enemy_id) {
  ENEMY enemy;
  char type_str[32] = "";
  POSITION start_pos = {0, 0};

  enemy.type = ENEMY_TYPE_UNKNOWN;
  enemy.last_teleport_frame = 0;
  enemy.chase_radius = 10;
  enemy.is_chasing = FALSE;
  enemy.direction = rand() % 2 == 0 ? DIR_FORWARD : DIR_BACKWARD;

  sscanf(line, "%31s (%d,%d) (%d,%d) (%d,%d) %d %d", type_str, &enemy.pos1.x,
         &enemy.pos1.y, &enemy.pos2.x, &enemy.pos2.y, &start_pos.x,
         &start_pos.y, &enemy.speed, &enemy.chase_radius);

  // Validate and clamp speed
  int verified_speed = MIN(MAX(enemy.speed, 1), 5);
  if (verified_speed != enemy.speed) {
    fprintf(stderr, "Warning: Enemies[%d] speed clamped to %d.\n", enemy_id,
            verified_speed);
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
    }
  } else if (strcmp(type_str, "PATROL") == 0) {
    if (enemy.pos1.y == enemy.pos2.y) {
      enemy.type = ENEMY_PATROL_HORIZONTAL;
    } else if (enemy.pos1.x == enemy.pos2.x) {
      enemy.type = ENEMY_PATROL_VERTICAL;
    }
  }

  if (enemy.type == ENEMY_TYPE_UNKNOWN) {
    fprintf(stderr, "Warning: Enemies[%d] with unknown or invalid type.\n",
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

PASSIVE_OBSTACLE LoadPassiveObstacle(const char *line, int obs_id) {
  PASSIVE_OBSTACLE obs;
  char type_str[32] = "";

  obs.teleport_id = -1;
  obs.teleport_target_id = -1;
  obs.teleport_cooldown = 0;

  sscanf(line, "%31s (%d,%d) (%d,%d) %d %d", type_str, &obs.pos1.x, &obs.pos1.y,
         &obs.pos2.x, &obs.pos2.y, &obs.teleport_id, &obs.teleport_target_id);

  if (strcmp(type_str, "TELEPORT") == 0) {
    obs.type = PASSIVE_TELEPORT_PAD;
  } else if (strcmp(type_str, "STICKY") == 0) {
    obs.type = PASSIVE_STICKY_FLOOR;
  } else if (strcmp(type_str, "GRASS") == 0) {
    obs.type = PASSIVE_TALL_GRASS;
  } else {
    obs.type = PASSIVE_TYPE_UNKNOWN;
    fprintf(stderr, "Warning: PassiveObstacles[%d] unknown type.\n", obs_id);
  }

  return obs;
}

static void InitLevelArrays(LEVEL *level) {
  level->totems = NULL;
  level->obstacles = NULL;
  level->enemies = NULL;
  level->aggressive_obstacles = NULL;
  level->powerups = NULL;
  level->passive_obstacles = NULL;
  level->num_totems = 0;
  level->num_obstacles = 0;
  level->num_enemies = 0;
  level->num_aggressive_obstacles = 0;
  level->num_powerups = 0;
  level->num_passive_obstacles = 0;
  level->total_totems_score = 0;
}

static int ParseConfigLine(const char *line, LEVEL *level) {
  sscanf(line, "TIME=%d", &level->time_limit);
  sscanf(line, "LIFES=%d", &level->lifes);
  sscanf(line, "WIDTH=%d", &level->width);
  sscanf(line, "HEIGHT=%d", &level->height);
  sscanf(line, "START_X=%d", &level->start_pos.x);
  sscanf(line, "START_Y=%d", &level->start_pos.y);
  return TRUE;
}

static int AddTotemEntity(const char *line, LEVEL *level) {
  TOTEM new_totem;
  new_totem.is_collected = FALSE;
  sscanf(line, "(%d,%d) %d", &new_totem.pos.x, &new_totem.pos.y,
         &new_totem.points);
  level->totems = (TOTEM *)AddLevelEntity(level->totems, sizeof(TOTEM),
                                          &level->num_totems, &new_totem);

  level->total_totems_score += new_totem.points;

  return (level->totems != NULL);
}

static int AddObstacleEntity(const char *line, LEVEL *level) {
  OBSTACLE new_obstacle;
  sscanf(line, "(%d,%d) (%d,%d)", &new_obstacle.pos1.x, &new_obstacle.pos1.y,
         &new_obstacle.pos2.x, &new_obstacle.pos2.y);
  level->obstacles = (OBSTACLE *)AddLevelEntity(
      level->obstacles, sizeof(OBSTACLE), &level->num_obstacles, &new_obstacle);
  return (level->obstacles != NULL);
}

static int AddEnemyEntity(const char *line, LEVEL *level) {
  ENEMY enemy = LoadEnemy(line, level->num_enemies);
  level->enemies = (ENEMY *)AddLevelEntity(level->enemies, sizeof(ENEMY),
                                           &level->num_enemies, &enemy);
  return (level->enemies != NULL);
}

static int AddAggObstEntity(const char *line, LEVEL *level) {
  AGGRESSIVE_OBSTACLE obs =
      LoadAggressiveObstacle(line, level->num_aggressive_obstacles);
  level->aggressive_obstacles = (AGGRESSIVE_OBSTACLE *)AddLevelEntity(
      level->aggressive_obstacles, sizeof(AGGRESSIVE_OBSTACLE),
      &level->num_aggressive_obstacles, &obs);
  return (level->aggressive_obstacles != NULL);
}

static int AddPowerupEntity(const char *line, LEVEL *level) {
  POWERUP p = LoadPowerup(line, level->num_powerups);
  level->powerups = (POWERUP *)AddLevelEntity(level->powerups, sizeof(POWERUP),
                                              &level->num_powerups, &p);
  return (level->powerups != NULL);
}

static int AddPassiveObstEntity(const char *line, LEVEL *level) {
  PASSIVE_OBSTACLE obs =
      LoadPassiveObstacle(line, level->num_passive_obstacles);
  level->passive_obstacles = (PASSIVE_OBSTACLE *)AddLevelEntity(
      level->passive_obstacles, sizeof(PASSIVE_OBSTACLE),
      &level->num_passive_obstacles, &obs);
  return (level->passive_obstacles != NULL);
}

static int ProcessLevelSection(const char *line, const char *section,
                               LEVEL *level) {
  if (strcmp(section, "[CONFIG]") == 0)
    return ParseConfigLine(line, level);
  if (strcmp(section, "[TOTEMS]") == 0)
    return AddTotemEntity(line, level);
  if (strcmp(section, "[OBSTACLES]") == 0)
    return AddObstacleEntity(line, level);
  if (strcmp(section, "[ENEMIES]") == 0)
    return AddEnemyEntity(line, level);
  if (strcmp(section, "[AGGRESSIVE_OBSTACLES]") == 0)
    return AddAggObstEntity(line, level);
  if (strcmp(section, "[POWERUPS]") == 0)
    return AddPowerupEntity(line, level);
  if (strcmp(section, "[PASSIVE_OBSTACLES]") == 0)
    return AddPassiveObstEntity(line, level);
  return TRUE;
}

LEVEL *LoadLevel(const char *filename) {
  LEVEL *level = (LEVEL *)malloc(sizeof(LEVEL));
  if (!level)
    return NULL;

  char *lname = GetLevelName(filename);
  strncpy(level->level_name, lname, MAX_LEVEL_NAME_LEN - 1);
  level->level_name[MAX_LEVEL_NAME_LEN - 1] = '\0';

  FILE *file = fopen(filename, "r");
  if (!file) {
    free(level);
    return NULL;
  }

  InitLevelArrays(level);
  char line[MAX_LINE_LEN];
  char *section = NULL;

  while (fgets(line, MAX_LINE_LEN, file)) {
    line[strcspn(line, "\n")] = 0;
    if (!strlen(line) || line[0] == '#')
      continue;

    if (line[0] == '[' && line[strlen(line) - 1] == ']') {
      free(section);
      section = strdup(line);
      if (!section) {
        FreeLevel(level);
        fclose(file);
        return NULL;
      }
      continue;
    }

    if (!section) {
      continue;
    }

    int processed_section = ProcessLevelSection(line, section, level);
    if (!processed_section) {
      free(section);
      FreeLevel(level);
      fclose(file);
      return NULL;
    }
  }

  free(section);
  fclose(file);
  return level;
}
