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
    free(level->aggressive_obstacles);
    free(level);
  }
}

ENEMY LoadEnemy(const char *line, int enemy_id) {
  ENEMY enemy;
  char type_str[32] = "";
  
  // Initialize new fields
  enemy.last_teleport_frame = 0;
  enemy.chase_radius = 10;  // Default chase radius
  
  // Try to parse with type prefix first
  int parsed = sscanf(line, "%31s (%d,%d) (%d,%d) %d %d %d", 
                      type_str, &enemy.pos1.x, &enemy.pos1.y,
                      &enemy.pos2.x, &enemy.pos2.y, &enemy.speed, 
                      &enemy.direction, &enemy.chase_radius);
  
  // If no type prefix or parsing failed, try old format (backward compatibility)
  if (parsed < 6 || type_str[0] == '(') {
    sscanf(line, "(%d,%d) (%d,%d) %d %d", &enemy.pos1.x, &enemy.pos1.y,
           &enemy.pos2.x, &enemy.pos2.y, &enemy.speed, &enemy.direction);
    strcpy(type_str, "PATROL");
  }
  
  // Set default direction if not provided
  if (enemy.direction != DIR_BACKWARD && enemy.direction != DIR_FORWARD) {
    enemy.direction = DIR_FORWARD;
  }
  
  // Validate and clamp speed
  int verified_speed = MIN(MAX(enemy.speed, 1), 5);
  if (verified_speed != enemy.speed) {
    fprintf(stderr,
            "Warning: Enemies[%d] speed %d out of bounds. Clamped to %d.\n",
            enemy_id, enemy.speed, verified_speed);
    enemy.speed = verified_speed;
  }
  
  // Set starting position
  enemy.current_pos = enemy.direction == DIR_BACKWARD ? enemy.pos2 : enemy.pos1;
  
  // Determine enemy type
  if (strcmp(type_str, "RANDOM") == 0 || strcmp(type_str, "RANDOM_WALKER") == 0) {
    enemy.type = ENEMY_RANDOM_WALKER;
    // pos1 and pos2 define the bounding box for random walking
  } else if (strcmp(type_str, "CHASER") == 0) {
    enemy.type = ENEMY_CHASER;
    // pos1 and pos2 define the area where chaser operates
    // chase_radius defines how close player needs to be
    if (parsed < 8) {
      enemy.chase_radius = 10;  // Default
    }
  } else if (strcmp(type_str, "TELEPORTER") == 0) {
    enemy.type = ENEMY_TELEPORTER;
    // pos1 and pos2 define the bounding box for teleportation
  } else if (strcmp(type_str, "PATROL") == 0) {
    // Determine patrol type based on positions
    if (enemy.pos1.y == enemy.pos2.y) {
      enemy.type = ENEMY_PATROL_HORIZONTAL;
    } else if (enemy.pos1.x == enemy.pos2.x) {
      enemy.type = ENEMY_PATROL_VERTICAL;
    } else {
      enemy.type = ENEMY_TYPE_UNKNOWN;
      fprintf(stderr, "Warning: Enemies[%d] with unknown patrol type.\n",
              enemy_id);
    }
  } else {
    // Legacy format: determine type from positions
    if (enemy.pos1.y == enemy.pos2.y) {
      enemy.type = ENEMY_PATROL_HORIZONTAL;
    } else if (enemy.pos1.x == enemy.pos2.x) {
      enemy.type = ENEMY_PATROL_VERTICAL;
    } else {
      enemy.type = ENEMY_TYPE_UNKNOWN;
      fprintf(stderr, "Warning: Enemies[%d] with unknown patrol type.\n",
              enemy_id);
    }
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
  level->aggressive_obstacles = NULL;
  level->num_totems = 0;
  level->num_obstacles = 0;
  level->num_enemies = 0;
  level->num_aggressive_obstacles = 0;

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
    } else if (strcmp(current_section, "[AGGRESSIVE_OBSTACLES]") == 0) {
      AGGRESSIVE_OBSTACLE new_obs;
      char type_str[32] = "";
      char dir_str[16] = "";
      
      // Parse: TYPE (x,y) [direction length interval] for fire jets
      // or: TYPE (x,y) for spikes
      int parsed = sscanf(line, "%31s (%d,%d) %15s %d %d", 
                          type_str, &new_obs.pos.x, &new_obs.pos.y,
                          dir_str, &new_obs.fire_length, &new_obs.fire_interval);
      
      if (parsed >= 3) {
        if (strcmp(type_str, "SPIKE") == 0) {
          new_obs.type = AGGRESSIVE_SPIKE;
          new_obs.fire_direction = FIREJET_UP;  // Not used for spikes
          new_obs.fire_length = 0;
          new_obs.fire_interval = 0;
          new_obs.last_fire_frame = 0;
          new_obs.is_active = 0;
        } else if (strcmp(type_str, "FIREJET") == 0) {
          new_obs.type = AGGRESSIVE_FIREJET;
          
          // Parse direction
          if (strcmp(dir_str, "UP") == 0) {
            new_obs.fire_direction = FIREJET_UP;
          } else if (strcmp(dir_str, "DOWN") == 0) {
            new_obs.fire_direction = FIREJET_DOWN;
          } else if (strcmp(dir_str, "LEFT") == 0) {
            new_obs.fire_direction = FIREJET_LEFT;
          } else if (strcmp(dir_str, "RIGHT") == 0) {
            new_obs.fire_direction = FIREJET_RIGHT;
          } else {
            new_obs.fire_direction = FIREJET_UP;
          }
          
          // Set defaults if not provided
          if (parsed < 5) {
            new_obs.fire_length = 5;
          }
          if (parsed < 6) {
            new_obs.fire_interval = 60;  // 3 seconds at 20 FPS
          }
          
          new_obs.last_fire_frame = 0;
          new_obs.is_active = 0;
        } else {
          continue;  // Unknown type, skip this line
        }
        
        level->aggressive_obstacles = (AGGRESSIVE_OBSTACLE *)AddLevelEntity(
            level->aggressive_obstacles, sizeof(AGGRESSIVE_OBSTACLE),
            &level->num_aggressive_obstacles, &new_obs);
        if (level->aggressive_obstacles == NULL) {
          free(current_section);
          FreeLevel(level);
          fclose(file);
          return NULL;
        }
      }
    }
  }

  free(current_section);
  fclose(file);
  return level;
}
