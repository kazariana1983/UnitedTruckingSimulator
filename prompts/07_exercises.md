# Prompt 07 - Exercise Framework Engineer

Implement exercise definitions for:
1. Straight-line backing
2. Offset backing left
3. Offset backing right
4. 90-degree alley dock

Do not hard-code exercise rules into maps.
Create a data-driven ExerciseDefinition containing starting tractor/trailer pose, goal, permitted/prohibited boundaries, cones, goal area, time limit, scoring-rule references, reset position, and completion conditions.

Track boundary crossings, cone contact, collisions, pull-ups, stops, time, final positions, and final alignment.
Provide visual debug overlays for invisible scoring zones and unique identifiers for boundaries.
Focus on accurate event detection, not final official CDL penalties.
