#include "ranking.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int compare_scores(const void *a, const void *b) {
  const RANKING_ENTRY *entry_a = (const RANKING_ENTRY *)a;
  const RANKING_ENTRY *entry_b = (const RANKING_ENTRY *)b;
  
  // Sort in descending order (highest score first)
  return entry_b->final_score - entry_a->final_score;
}

int CalculateFinalScore(GAMESTATE *state) {
  int time_left = MAX(0, state->level->time_limit - state->time_elapsed);

  int final_score = state->score +
                    (state->lifes_left * FINAL_SCORE_LIFE_BONUS) +
                    (time_left * FINAL_SCORE_TIME_BONUS);

  return final_score;
}

int SaveRanking(const char *level_name, GAMESTATE *state) {
  // Load all existing rankings
  RANKING_ENTRY all_rankings[MAX_TOTAL_RANKINGS];
  int total_count = LoadRankings(all_rankings, MAX_TOTAL_RANKINGS);
  
  // Create new entry
  RANKING_ENTRY new_entry;
  strncpy(new_entry.level_name, level_name, MAX_LEVEL_NAME_LEN - 1);
  new_entry.level_name[MAX_LEVEL_NAME_LEN - 1] = '\0';
  
  new_entry.totems_score = state->score;
  new_entry.lives_left = state->lifes_left;
  new_entry.time_left = state->level->time_limit - state->time_elapsed;
  if (new_entry.time_left < 0) {
    new_entry.time_left = 0;
  }
  new_entry.final_score = CalculateFinalScore(state);
  new_entry.timestamp = time(NULL);
  
  // Add new entry to the array
  if (total_count < MAX_TOTAL_RANKINGS) {
    all_rankings[total_count++] = new_entry;
  } else {
    // Array is full, replace the last entry
    all_rankings[MAX_TOTAL_RANKINGS - 1] = new_entry;
  }
  
  // Count entries per level and keep only top 10 for each level
  RANKING_ENTRY filtered_rankings[MAX_TOTAL_RANKINGS];
  int filtered_count = 0;
  
  // Process each unique level
  for (int i = 0; i < total_count; i++) {
    const char *current_level = all_rankings[i].level_name;
    
    // Skip if we already processed this level
    int already_processed = 0;
    for (int j = 0; j < i; j++) {
      if (strcmp(all_rankings[j].level_name, current_level) == 0) {
        already_processed = 1;
        break;
      }
    }
    if (already_processed) {
      continue;
    }
    
    // Collect all entries for this level
    RANKING_ENTRY level_entries[MAX_TOTAL_RANKINGS];
    int level_count = 0;
    for (int j = 0; j < total_count; j++) {
      if (strcmp(all_rankings[j].level_name, current_level) == 0) {
        level_entries[level_count++] = all_rankings[j];
      }
    }
    
    // Sort entries for this level by score (descending)
    qsort(level_entries, level_count, sizeof(RANKING_ENTRY), compare_scores);
    
    // Keep only top 10 entries for this level
    int entries_to_keep = (level_count < MAX_RANKINGS_PER_LEVEL) ? level_count : MAX_RANKINGS_PER_LEVEL;
    for (int j = 0; j < entries_to_keep; j++) {
      if (filtered_count < MAX_TOTAL_RANKINGS) {
        filtered_rankings[filtered_count++] = level_entries[j];
      }
    }
  }
  
  // Write all filtered rankings back to file in plaintext format
  FILE *file = fopen(RANKINGS_FILE, "w");
  if (!file) {
    return -1;
  }
  
  for (int i = 0; i < filtered_count; i++) {
    fprintf(file, "%s|%d|%d|%d|%d|%ld\n",
            filtered_rankings[i].level_name,
            filtered_rankings[i].final_score,
            filtered_rankings[i].totems_score,
            filtered_rankings[i].lives_left,
            filtered_rankings[i].time_left,
            (long)filtered_rankings[i].timestamp);
  }
  
  fclose(file);
  return 0;
}

int LoadRankings(RANKING_ENTRY *rankings, int max_entries) {
  FILE *file = fopen(RANKINGS_FILE, "r");
  if (!file) {
    return 0;
  }
  
  int count = 0;
  char line[512];
  
  while (count < max_entries && fgets(line, sizeof(line), file) != NULL) {
    RANKING_ENTRY entry;
    long timestamp_long;
    
    // Parse the plaintext format: level_name|final_score|totems_score|lives_left|time_left|timestamp
    int parsed = sscanf(line, "%63[^|]|%d|%d|%d|%d|%ld",
                        entry.level_name,
                        &entry.final_score,
                        &entry.totems_score,
                        &entry.lives_left,
                        &entry.time_left,
                        &timestamp_long);
    
    if (parsed == 6) {
      entry.level_name[MAX_LEVEL_NAME_LEN - 1] = '\0';
      entry.timestamp = (time_t)timestamp_long;
      rankings[count] = entry;
      count++;
    }
  }
  
  fclose(file);
  return count;
}

int GetLevelRankings(const char *level_name, RANKING_ENTRY *rankings, int max_entries) {
  RANKING_ENTRY all_rankings[MAX_TOTAL_RANKINGS];
  int total_count = LoadRankings(all_rankings, MAX_TOTAL_RANKINGS);
  
  int level_count = 0;
  for (int i = 0; i < total_count && level_count < max_entries; i++) {
    if (strcmp(all_rankings[i].level_name, level_name) == 0) {
      rankings[level_count] = all_rankings[i];
      level_count++;
    }
  }
  
  // Sort by score (descending)
  qsort(rankings, level_count, sizeof(RANKING_ENTRY), compare_scores);
  
  return level_count;
}
