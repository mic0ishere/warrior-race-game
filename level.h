#ifndef LEVEL_H
#define LEVEL_H

#include "types.h"

// Free level memory
void FreeLevel(LEVEL *level);

// Load enemy from level file line
ENEMY LoadEnemy(const char *line, int enemy_id);

// Load level from file
LEVEL *LoadLevel(const char *filename);

// Generic function to add entities to level arrays
void *AddLevelEntity(void *array, size_t entry_size, int *num_entries,
                     const void *new_entry);

#endif // LEVEL_H
