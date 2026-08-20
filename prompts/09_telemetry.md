# Prompt 09 - Telemetry Engineer

Implement telemetry sufficient to reconstruct student and vehicle behavior.

Record session identifiers, tractor pose/velocity/speed/steering, trailer pose/articulation/rear and axle positions, raw and normalized student inputs, gear/parking brake, and exercise events including pull-up, stop, collision, cone contact, boundary crossing, reset, start, and completion.

Requirements:
- configurable sample frequency
- minimal performance impact
- telemetry frame + event stream
- local temporary storage
- upload queue
- retry handling
- backend synchronization
- simulator remains functional offline
- format supports future session replay

Document expected data volume for a 10-minute attempt.
