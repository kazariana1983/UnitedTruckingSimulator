# Prompt 03 - Unreal Foundation Engineer

Implement ONLY foundational Unreal architecture. Do not implement realistic vehicle physics yet.

Create/configure:
1. Game instance
2. Student session manager
3. Exercise manager
4. Telemetry subsystem
5. Scoring subsystem interface
6. Input abstraction layer
7. Vehicle base interfaces
8. Simulator configuration system
9. Backend API client interface
10. Logging
11. Debug HUD

Requirements:
- C++ core logic
- Blueprint exposure only for configuration/UI where appropriate
- avoid global mutable state
- use Unreal subsystems where appropriate
- systems independently testable
- exercise logic not embedded in vehicle classes
- scoring not hard-coded in maps
- hardware-specific input not hard-coded into tractor

Before changes: inspect repository, describe changes, identify affected modules.
Implement in small logical changes. Compile, run tests, report errors/warnings, and document unresolved issues.
