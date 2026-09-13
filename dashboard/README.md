# `dashboard/` — Instructor Dashboard (React + TypeScript)

Status: Directory scaffold only. No application code yet.

Owner: Dashboard Engineer, starting at Master Delivery Plan `P1-08` (dashboard skeleton), once `docs/TECHNICAL_ARCHITECTURE.md` §9 is approved.

Module map (Student List, Simulator Station Status, Current Attempt, Attempt History, Scores, Common Mistakes, Instructor Notes) and required loading/empty/failure/offline/unauthorized states: `docs/TECHNICAL_ARCHITECTURE.md` §9. Requirement coverage: `docs/REQUIREMENT_TRACEABILITY_MATRIX.md` (FR-UI-*).

This application consumes backend APIs only (`FR-UI-005`) — no direct database access, no simulator access, no substitute scoring or backend logic.
