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

void WriteRankingEntry(FILE *file, RANKING_ENTRY *entry) {
  fprintf(file, "%s|%d|%d|%d|%d|%ld\n", entry->level_name, entry->final_score,
          entry->totems_score, entry->lives_left, entry->time_left,
          (long)entry->timestamp);
}

int SaveRanking(const char *level_name, GAMESTATE *state) {
  // Create new entry
  RANKING_ENTRY new_entry;
  strncpy(new_entry.level_name, level_name, MAX_LEVEL_NAME_LEN - 1);
  new_entry.level_name[MAX_LEVEL_NAME_LEN - 1] = '\0';

  new_entry.totems_score = state->score;
  new_entry.lives_left = state->lifes_left;
  new_entry.time_left = MAX(0, state->level->time_limit - state->time_elapsed);
  new_entry.final_score = CalculateFinalScore(state);
  new_entry.timestamp = time(NULL);

  // Load rankings for current level
  RANKING_ENTRY rankings[MAX_RANKINGS_PER_LEVEL + 1];
  int num_rankings =
      GetLevelRankings(level_name, rankings, MAX_RANKINGS_PER_LEVEL);

  // Add new entry to rankings
  rankings[num_rankings] = new_entry;
  num_rankings++;

  // Sort new rankings by score
  qsort(rankings, num_rankings, sizeof(RANKING_ENTRY), compare_scores);

  // Load all existing rankings
  RANKING_ENTRY all_rankings[MAX_TOTAL_RANKINGS];
  int total_count = LoadRankings(all_rankings, MAX_TOTAL_RANKINGS);

  // Write all filtered rankings back to file in plaintext format
  FILE *file = fopen(RANKINGS_FILE, "w");
  if (!file) {
    return -1;
  }

  // Write top rankings for the current level
  for (int i = 0; i < MIN(MAX_RANKINGS_PER_LEVEL, num_rankings); i++) {
    WriteRankingEntry(file, &rankings[i]);
  }

  // Write back all other rankings for different levels
  for (int i = 0; i < total_count; i++) {
    if (strcmp(all_rankings[i].level_name, level_name) != 0) {
      WriteRankingEntry(file, &all_rankings[i]);
    }
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

    // Parse the plaintext format:
    // level_name|final_score|totems_score|lives_left|time_left|timestamp
    int parsed = sscanf(line, "%63[^|]|%d|%d|%d|%d|%ld", entry.level_name,
                        &entry.final_score, &entry.totems_score,
                        &entry.lives_left, &entry.time_left, &timestamp_long);

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

int GetLevelRankings(const char *level_name, RANKING_ENTRY *rankings,
                     int max_entries) {
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
