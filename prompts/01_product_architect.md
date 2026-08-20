# Prompt 01 - Product Architect

You are the principal product architect for a commercial CDL Class A trucking-school simulator.

Build a software requirements specification for an MVP used by a real trucking school.

The product is NOT an entertainment game. Its purpose is to teach and evaluate CDL students practicing tractor-trailer backing maneuvers.

Target platform:
- Windows PC
- Unreal Engine 5
- physical steering wheel
- accelerator and brake pedals
- optional clutch
- optional H-pattern/range-splitter shifter
- single screen initially; future triple-monitor support

MVP vehicle:
- Class 8 day-cab tractor
- tandem rear axles
- 53-foot dry van trailer
- fifth-wheel articulated connection

MVP exercises:
1. Straight-line backing
2. Offset backing left
3. Offset backing right
4. 90-degree alley dock

MVP features:
- student login
- exercise selection
- training yard
- vehicle reset
- first-person cab camera
- left/right mirror cameras
- exterior instructor camera
- collision detection
- boundary detection
- pull-up detection
- stop detection
- trailer articulation measurement
- steering angle
- tractor/trailer position
- speed
- brake/throttle input
- timer
- automatic deterministic scoring
- attempt history
- instructor dashboard
- telemetry recording
- AI coaching architecture

Exclude for now:
- open-world map
- highway driving
- AI traffic
- multiplayer
- VR
- weather
- damage simulation
- vehicle customization
- cargo
- career/game systems

Create a detailed software requirements specification containing:
1. Product objectives
2. User types
3. Functional requirements
4. Non-functional requirements
5. Unreal Engine systems
6. Backend systems
7. Database entities
8. Telemetry schema
9. Scoring architecture
10. Hardware abstraction architecture
11. AI coaching architecture
12. API requirements
13. Security requirements
14. Logging
15. Testing strategy
16. Acceptance criteria
17. Development milestones
18. Major technical risks

Where requirements are unknown, create configurable values instead of permanent assumptions.
Design for additional trucks, trailers, maneuvers, schools, and simulator stations later.
Do not write production code yet.
