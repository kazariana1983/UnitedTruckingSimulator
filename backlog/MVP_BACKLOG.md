# MVP Backlog

## Milestone 0 - Definition
- [ ] Complete software requirements specification
- [ ] Approve architecture
- [ ] Populate initial ground-truth vehicle measurements
- [ ] Populate training-yard dimensions
- [ ] Define human-approved practice scoring profile

## Milestone 1 - Simulator Foundation
- [ ] Create Unreal project
- [ ] Implement session manager
- [ ] Implement exercise manager
- [ ] Implement telemetry subsystem interface
- [ ] Implement scoring subsystem interface
- [ ] Implement input abstraction
- [ ] Implement configuration system
- [ ] Implement debug HUD

## Milestone 2 - Hardware + Vehicle
- [ ] Keyboard developer controls
- [ ] Generic wheel/pedal input
- [ ] Calibration UI
- [ ] Tractor low-speed physics
- [ ] 53 ft trailer physics
- [ ] Fifth-wheel articulation
- [ ] Mirror/camera system
- [ ] Physics debug overlays

## Milestone 3 - Training Exercises
- [ ] Straight-line backing
- [ ] Offset backing left
- [ ] Offset backing right
- [ ] 90-degree alley dock
- [ ] Boundary event detection
- [ ] Cone/collision event detection
- [ ] Pull-up detection
- [ ] Completion detection

## Milestone 4 - Data + Scoring
- [ ] Telemetry frame schema
- [ ] Event stream
- [ ] Local offline cache
- [ ] Deterministic scoring engine
- [ ] Telemetry replay scoring test

## Milestone 5 - Platform
- [ ] FastAPI backend
- [ ] PostgreSQL database
- [ ] Authentication and roles
- [ ] Simulator station heartbeat
- [ ] Attempt upload
- [ ] Instructor dashboard
- [ ] Student history
- [ ] Attempt detail

## Milestone 6 - Validation
- [ ] Independent code review
- [ ] CDL instructor realism review
- [ ] Physics tuning
- [ ] Scoring validation
- [ ] Student pilot

## Milestone 7 - AI Coaching
- [ ] Backend coaching service
- [ ] Post-attempt coaching
- [ ] Live concise coaching
- [ ] Historical attempt comparison
- [ ] Guardrails against invented telemetry/regulations
