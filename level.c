#include "level.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *AddLevelEntity(void *array, size_t entry_size, int *num_entries,
                     const void *new_entry) {
  void *resized_array = realloc(array, (*num_entries + 1) * entry_size);

  if (resized_array == NULL) {
    return NULL;
  }

  memcpy((char *)resized_array + (*num_entries) * entry_size, new_entry, entry_size);
  (*num_entries)++;

  return resized_array;
}

void FreeLevel(LEVEL *level) {
  if (level != NULL) {
    free(level->totems);
    free(level->obstacles);
    free(level->enemies);
    free(level);
  }
}

ENEMY LoadEnemy(const char *line, int enemy_id) {
  ENEMY enemy;
  sscanf(line, "(%d,%d) (%d,%d) %d %d", &enemy.pos1.x, &enemy.pos1.y,
         &enemy.pos2.x, &enemy.pos2.y, &enemy.speed, &enemy.direction);

  if (enemy.direction != DIR_BACKWARD && enemy.direction != DIR_FORWARD) {
    enemy.direction = DIR_FORWARD;
    fprintf(stderr,
            "Warning: Enemies[%d] has invalid direction. Set to FORWARD.\n",
            enemy_id);
  }

  int verified_speed = MIN(MAX(enemy.speed, 1), 5);
  if (verified_speed != enemy.speed) {
    fprintf(stderr,
            "Warning: Enemies[%d] speed %d out of bounds. Clamped to %d.\n",
            enemy_id, enemy.speed, verified_speed);
    enemy.speed = verified_speed;
  }

  enemy.current_pos = enemy.direction == DIR_BACKWARD ? enemy.pos2 : enemy.pos1;

  if (enemy.pos1.y == enemy.pos2.y) {
    enemy.type = ENEMY_PATROL_HORIZONTAL;
  } else if (enemy.pos1.x == enemy.pos2.x) {
    enemy.type = ENEMY_PATROL_VERTICAL;
  } else {
    enemy.type = ENEMY_TYPE_UNKNOWN;
    fprintf(stderr, "Warning: Enemies[%d] with unknown patrol type.\n",
            enemy_id);
  }

  return enemy;
}

LEVEL *LoadLevel(const char *filename) {
  LEVEL *level = (LEVEL *)malloc(sizeof(LEVEL));
  
  if (level == NULL) {
    return NULL;
  }

  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    free(level);
    return NULL;
  }

  level->totems = NULL;
  level->obstacles = NULL;
  level->enemies = NULL;
  level->num_totems = 0;
  level->num_obstacles = 0;
  level->num_enemies = 0;

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
      sscanf(line, "LIVES=%d", &level->lives);
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
    }
  }

  free(current_section);
  fclose(file);
  return level;
}
