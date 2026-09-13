# `contracts/` — Shared Versioned Schema

Status: Directory scaffold only. No schema files yet.

Owner: Lead Software Architect + Telemetry Engineer, Master Delivery Plan `P1-04`, once `docs/TECHNICAL_ARCHITECTURE.md` is approved.

This directory will hold the versioned, cross-application definitions that `simulator/`, `backend/`, and `dashboard/` must all agree on byte-for-byte: telemetry frame schema, domain event taxonomy, configuration-profile envelope shape, and backend API request/response DTOs. Its purpose is to make a schema change one reviewable diff touched by every consumer in the same PR, rather than three independently-drifting copies (see `docs/TECHNICAL_ARCHITECTURE.md` §2 for the monorepo rationale).

The C++ shape of these contracts is drafted now in `simulator/Source/UTSCore/Public/UTS/Common/UTSCommonTypes.h` and the per-module interface headers alongside it; this directory is where that shape becomes the single source of truth once a schema language/format is chosen (JSON Schema, protobuf, or hand-written per-language types — not yet decided, see `docs/ARCHITECTURE_OPEN_DECISIONS.md`).
