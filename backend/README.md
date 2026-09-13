# `backend/` — Platform Services (FastAPI + PostgreSQL)

Status: Directory scaffold only. No application code yet.

Owner: Backend Engineer, starting at Master Delivery Plan `P1-06` (backend skeleton) and `P1-07` (database foundation), once `docs/TECHNICAL_ARCHITECTURE.md` §8 is approved.

Module map, API groups, entity list, and per-module responsibility/failure/test breakdown: `docs/TECHNICAL_ARCHITECTURE.md` §8. Requirement coverage: `docs/REQUIREMENT_TRACEABILITY_MATRIX.md` (FR-API-*, FR-AI-* for the `coaching` module).

Nothing under this directory may implement scoring or physics logic — the backend only stores and serves what the simulator's deterministic Scoring module (or its own replay of the same algorithm) produced, per `NFR-002`.
