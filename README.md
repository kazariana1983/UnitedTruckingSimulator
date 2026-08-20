# CDL Simulator AI Build Package

Purpose: bootstrap an MVP CDL Class A training simulator project.

MVP scope:
- Windows PC
- Unreal Engine 5
- Class 8 day-cab tractor + 53 ft dry van
- Steering wheel/pedals support
- Straight-line backing
- Offset backing left/right
- 90-degree alley dock
- Deterministic scoring
- Telemetry
- FastAPI/PostgreSQL backend
- React/TypeScript instructor dashboard
- AI coaching layer that never controls physics or official scoring

## Agent roles
1. Product Architect
2. Lead Software Architect
3. Unreal Foundation Engineer
4. Tractor Physics Engineer
5. Trailer Physics Engineer
6. Hardware/Input Engineer
7. Exercise Framework Engineer
8. Scoring Engineer
9. Telemetry Engineer
10. Backend Engineer
11. Dashboard Engineer
12. AI Instructor Engineer
13. Independent QA Reviewer
14. Physics Diagnostic Reviewer

## Operating rule
Every agent must treat `ground_truth/SIMULATOR_GROUND_TRUTH.md` as authoritative. AI agents must not invent CDL regulations, official scoring rules, vehicle dimensions, or validated physics facts.

## Recommended execution order
1. prompts/01_product_architect.md
2. prompts/02_software_architect.md
3. prompts/03_unreal_foundation.md
4. prompts/06_hardware_input.md
5. prompts/04_tractor_physics.md
6. prompts/05_trailer_physics.md
7. prompts/07_exercises.md
8. prompts/09_telemetry.md
9. prompts/08_scoring.md
10. prompts/10_backend.md
11. prompts/11_dashboard.md
12. prompts/13_qa_review.md
13. Human CDL instructor validation
14. prompts/12_ai_instructor.md
15. prompts/14_physics_diagnostics.md
