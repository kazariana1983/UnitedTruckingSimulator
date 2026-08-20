# Prompt 02 - Lead Software Architect

Read the approved software requirements specification and design the complete technical architecture.

Required technologies:
- Unreal Engine 5, C++ core, Blueprints for configuration/presentation only
- Python FastAPI
- PostgreSQL
- React + TypeScript
- LLM API only from backend services

Physics and scoring must never depend on an LLM.

Design modules for Unreal, backend, and dashboard. For every module specify responsibility, inputs, outputs, interfaces, dependencies, primary classes/components, failure cases, and tests.

Unreal modules:
- vehicle
- tractor physics
- trailer physics
- fifth-wheel coupling
- input abstraction
- camera/mirror system
- exercise manager
- boundary/collision detection
- telemetry
- scoring
- student session
- local cache
- backend client

Backend modules:
- authentication
- students
- instructors
- simulator stations
- exercises
- attempts
- telemetry
- scores
- coaching
- reporting

Dashboard:
- student list
- simulator station status
- current attempt
- attempt history
- scores
- common mistakes
- instructor notes

Also propose repository structure, branching strategy, coding standards, configuration strategy, logging, build/deployment, and a dependency-ordered implementation backlog.
Identify systems that must remain decoupled.
Do not begin implementation until architecture is internally consistent.
