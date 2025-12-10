#ifndef GAME_H
#define GAME_H

#include "types.h"

// Initialize game state
GAMESTATE *InitGame(void);

// Check if position is within bounds
int CheckWithinBounds(POSITION pos1, POSITION pos2, POSITION point);

// Check collision with obstacles
int CheckCollision(GAMESTATE *state, POSITION wanted_pos);

// Collect totems at player position
void CollectTotems(GAMESTATE *state);

// Move position in specified direction
void MoveInDirection(POSITION *pos, int direction, int amount);

// Get next enemy position
POSITION NextEnemyPosition(ENEMY *enemy);

// Move all enemies
void MoveEnemies(GAMESTATE *state);

// Check for player-enemy collision
void CheckPlayerEnemyCollision(GAMESTATE *state);

// Update fire jets and check player collision with aggressive obstacles
void UpdateAggressiveObstacles(GAMESTATE *state);

// Check if player is on fire jet's fire path
int IsOnFirePath(AGGRESSIVE_OBSTACLE *firejet, POSITION player_pos);

// Main game loop
void MainLoop(WIN *playwin, WIN *statwin, WIN *debugwin, GAMESTATE *state);

// Finish the game and display final status
void FinishGame(WIN *playwin, GAMESTATE *state);

#endif // GAME_H
