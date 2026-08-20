# Prompt 05 - Trailer Physics Engineer

Implement configurable 53-foot semitrailer dynamics attached to the existing Class 8 tractor. Primary goal: realistic backing behavior.

Implement:
- fifth-wheel pivot connection
- kingpin
- trailer mass and center of gravity
- tandem axles
- wheel behavior
- articulation
- forward/reverse tracking
- trailer swing
- off-tracking
- jackknife detection

Expose telemetry:
- tractor heading
- trailer heading
- articulation angle/rate
- kingpin position
- trailer rear-center/left/right positions
- trailer axle position

Do not artificially prevent jackknifing.
Create debug visualization for tractor/trailer paths, kingpin, axle paths, articulation, and projected movement.
Support future trailer lengths without rewriting the system.
Create repeatable validation tests and document Unreal physics assumptions.
