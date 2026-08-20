# Prompt 08 - Scoring Engineer

Design a deterministic scoring engine. It must NEVER rely on an LLM.

Create configurable rules for boundary encroachment, cone contact, collision, excessive pull-up, incorrect final position, excessive final angle, excessive speed, timeout, jackknife, and successful completion.

Every scoring event must contain timestamp, event type, severity, measured value, threshold, penalty, explanation, and evidence reference.

The same telemetry replay must always produce exactly the same score.
Support practice mode, school evaluation, jurisdiction-specific configuration, and instructor-defined exercises.
Do not claim any rule represents an official DMV/CDL examination unless explicitly supplied and human-validated.

Implement ScoreResult, ScoreEvent, ScoringRule, ScoringProfile, and ScoringEngine plus unit tests and telemetry replay determinism tests.
