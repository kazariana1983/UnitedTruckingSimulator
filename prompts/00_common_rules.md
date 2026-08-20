# Common Agent Rules

Before every task, apply these instructions:

Do not attempt to solve the entire project. Work only on the requested subsystem.

Before coding:
- inspect the current repository
- understand existing interfaces
- identify dependencies
- identify risks

Preserve working behavior.
Do not replace existing architecture without strong justification.
Do not invent business requirements.
Do not invent CDL regulations.
Prefer configurable parameters over unexplained hard-coded values.
Treat `ground_truth/SIMULATOR_GROUND_TRUTH.md` as authoritative.
Add tests for new deterministic logic.
Compile and test after implementation.
If something cannot be correctly validated in software, flag it for human review instead of pretending it is correct.
