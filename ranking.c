#include "ranking.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int CalculateFinalScore(GAMESTATE *state) {
  int time_left = state->level->time_limit - state->time_elapsed;
  if (time_left < 0) {
    time_left = 0;
  }
  
  int final_score = state->score + 
                    (state->lifes_left * LIFE_BONUS) + 
                    (time_left * TIME_BONUS);
  
  return final_score;
}

int SaveRanking(const char *level_name, GAMESTATE *state) {
  // Try to open for appending, create if doesn't exist
  FILE *file = fopen(RANKINGS_FILE, "ab");
  if (!file) {
    return -1;
  }
  
  RANKING_ENTRY entry;
  strncpy(entry.level_name, level_name, MAX_LEVEL_NAME_LEN - 1);
  entry.level_name[MAX_LEVEL_NAME_LEN - 1] = '\0';
  
  entry.totems_score = state->score;
  entry.lives_left = state->lifes_left;
  entry.time_left = state->level->time_limit - state->time_elapsed;
  if (entry.time_left < 0) {
    entry.time_left = 0;
  }
  entry.final_score = CalculateFinalScore(state);
  entry.timestamp = time(NULL);
  
  size_t written = fwrite(&entry, sizeof(RANKING_ENTRY), 1, file);
  fclose(file);
  
  return (written == 1) ? 0 : -1;
}

int LoadRankings(RANKING_ENTRY *rankings, int max_entries) {
  FILE *file = fopen(RANKINGS_FILE, "rb");
  if (!file) {
    return 0;
  }
  
  int count = 0;
  RANKING_ENTRY entry;
  
  while (count < max_entries && fread(&entry, sizeof(RANKING_ENTRY), 1, file) == 1) {
    rankings[count] = entry;
    count++;
  }
  
  fclose(file);
  return count;
}

static int compare_scores(const void *a, const void *b) {
  const RANKING_ENTRY *entry_a = (const RANKING_ENTRY *)a;
  const RANKING_ENTRY *entry_b = (const RANKING_ENTRY *)b;
  
  // Sort in descending order (highest score first)
  return entry_b->final_score - entry_a->final_score;
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
