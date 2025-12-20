# Power-Up System Testing Guide

This document provides a guide for testing the newly implemented power-up system.

## Power-Up Types

### 1. Shield (S) - Cyan Color
**Effect**: Negates the next enemy collision
**Testing Steps**:
1. Start level1.txt or test_powerups.txt
2. Navigate to and collect a Shield power-up (S)
3. Verify status bar shows "[Shield]"
4. Intentionally collide with an enemy
5. Verify: Shield is consumed, no life is lost, status bar updates

**Expected Behavior**:
- Shield icon appears in status bar after collection
- On enemy collision, shield is consumed instead of losing a life
- Shield disappears from status bar after use
- Player respawns normally after shield protects from hit

**Important Note**: Shield only protects against enemies (E), NOT environmental hazards like spikes or fire jets. This is intentional game design.

### 2. Double Points (D) - Yellow Color
**Effect**: Doubles all points earned for 10 seconds
**Testing Steps**:
1. Start a level with both power-ups and totems
2. Note current score
3. Collect a Double Points power-up (D)
4. Verify status bar shows "[2x Points:10s]" (counting down)
5. Collect a totem worth X points
6. Verify score increased by 2X points
7. Wait for timer to expire
8. Collect another totem
9. Verify score increases by normal X points

**Expected Behavior**:
- Timer displays and counts down from 10 seconds
- All totem collections during effect grant double points
- Effect automatically expires after 10 seconds
- Normal scoring resumes after expiration

### 3. Extra Life (L) - Green Color
**Effect**: Instantly grants one additional life
**Testing Steps**:
1. Start a level and note life count
2. Collect an Extra Life power-up (L)
3. Verify life count increases by 1 immediately
4. No status bar indicator (instant effect)

**Expected Behavior**:
- Life count increases immediately on collection
- Power-up disappears from map
- No timer or status indicator (instant effect)

### 4. Time Freeze (F) - Blue Color
**Effect**: Freezes all enemies for 5 seconds
**Testing Steps**:
1. Start a level with moving enemies
2. Observe enemy movement patterns
3. Collect a Time Freeze power-up (F)
4. Verify status bar shows "[Freeze:5s]" (counting down)
5. Observe all enemies are frozen (not moving)
6. Verify player can still move and collect totems
7. Wait for timer to expire
8. Verify enemies resume normal movement

**Expected Behavior**:
- Timer displays and counts down from 5 seconds
- All enemy types stop moving during effect
- Player movement and totem collection work normally
- Enemies resume movement after expiration

## Level Files with Power-Ups

### test_powerups.txt
Comprehensive test level with all power-up types:
- SHIELD at (5,5) and (30,5)
- DOUBLE at (20,20)
- LIFE at (35,15)
- FREEZE at (10,25)

### level1.txt
Original level enhanced with power-ups:
- SHIELD at (3,3)
- DOUBLE at (18,18)
- LIFE at (27,27)
- FREEZE at (15,5)

### level2.txt
Smaller level with basic power-ups:
- SHIELD at (2,2)
- DOUBLE at (12,12)
- LIFE at (22,22)

## Integration Testing

### Multiple Power-Ups Active Simultaneously
1. Collect Shield and Double Points
2. Verify both appear in status bar
3. Collect a totem - should get double points
4. Collide with enemy - should consume shield
5. Verify both effects work independently

### Power-Up Stacking
1. Collect multiple power-ups of different types
2. Verify all active effects show in status bar
3. Effects should expire independently based on their timers

### Edge Cases

#### Max Active Power-Ups
- Collect 4 timed power-ups (max limit)
- Attempt to collect a 5th
- Verify system handles gracefully (may not activate 5th)

#### Rapid Collection
- Collect multiple power-ups quickly
- Verify all are registered and activate correctly

#### End of Game with Active Power-Ups
- Activate power-ups and end the game (win/lose/quit)
- Verify no crashes or unexpected behavior

## Visual Verification

### Power-Up Symbols on Map
- S (Shield) - Cyan background
- D (Double Points) - Yellow background
- L (Extra Life) - Green background
- F (Time Freeze) - Blue background

### Status Bar Display
- Active power-ups show with format: [Name:Xs]
- Shield shows as: [Shield] (no timer)
- Multiple power-ups display on same line
- Timers count down in real-time (updates every second)

## Known Behaviors

- Power-ups disappear from map once collected
- Timed power-ups cannot be stacked (same type)
- Shield persists across level sections until used
- Extra Life is instant with no visual indicator in status bar
- Maximum 4 active timed power-ups at once

## Troubleshooting

If power-ups don't appear:
1. Check level file has [POWERUPS] section
2. Verify syntax: TYPE (x,y) format
3. Ensure coordinates are within level bounds
4. Check power-up types are spelled correctly (SHIELD, DOUBLE, LIFE, FREEZE)

If effects don't work:
1. Rebuild the game: `make rebuild`
2. Verify you're running the latest binary
3. Check stderr for any error messages
