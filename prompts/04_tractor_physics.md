# Prompt 04 - Tractor Physics Engineer

Implement the tractor portion of a Class 8 CDL training simulator in Unreal Engine 5.
Accuracy for low-speed maneuvering is more important than game feel.

All vehicle characteristics must be configurable and sourced from approved ground truth where available.

Model/configure:
- wheelbase
- front steering axle
- tandem drive axles
- vehicle mass
- center of gravity
- steering ratio
- maximum steering angle
- tire behavior
- rolling resistance
- engine torque
- idle behavior
- transmission ratios
- braking
- parking brake
- reverse
- low-speed throttle response

Primary performance target: 0-15 MPH yard maneuvering.

Requirements:
- stable at very low speed
- predictable steering
- realistic turning radius
- realistic forward/reverse response
- no arcade steering assistance
- no hidden auto-correction

Instrumentation:
- speed
- steering wheel angle
- road-wheel angle
- throttle
- brake
- current gear
- tractor yaw
- position
- wheel slip

Provide validation procedures for minimum turning radius, stopping distance, reverse control, steering response, and low-speed stability.
Do not implement trailer behavior. Document every approximation.
