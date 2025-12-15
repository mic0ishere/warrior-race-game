#ifndef RANKING_H
#define RANKING_H

#include "types.h"

// Calculate final score based on game state
int CalculateFinalScore(GAMESTATE *state);

// Save ranking entry to file
int SaveRanking(const char *level_name, GAMESTATE *state);

// Load all rankings from file
int LoadRankings(RANKING_ENTRY *rankings, int max_entries);

// Get rankings for a specific level
int GetLevelRankings(const char *level_name, RANKING_ENTRY *rankings, int max_entries);

#endif // RANKING_H
