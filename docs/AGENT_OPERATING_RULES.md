# Agent Operating Rules

Apply these rules to every coding task:

1. Work only on the requested subsystem.
2. Inspect the current repository before changing code.
3. Preserve working behavior unless a change is explicitly required.
4. Do not invent business requirements.
5. Do not invent CDL regulations or official testing requirements.
6. Prefer configurable parameters over hard-coded values.
7. Treat `ground_truth/SIMULATOR_GROUND_TRUTH.md` as authoritative.
8. Add tests for deterministic logic.
9. Compile/test after implementation.
10. Separate confirmed findings from hypotheses.
11. Flag anything requiring human validation.
12. Do not use an LLM for official scoring or vehicle physics decisions.
13. Keep exercise rules separate from maps and vehicle classes.
14. Keep hardware-specific code behind an abstraction layer.
15. Record meaningful telemetry for all training attempts.
