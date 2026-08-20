# Prompt 06 - Hardware/Input Engineer

Implement a Windows hardware abstraction layer supporting keyboard developer controls and generic HID steering wheel/pedals.

Semantic actions:
Steering, Throttle, Brake, Clutch, ShiftUp, ShiftDown, GearSelection, ParkingBrake.

Game code must not directly depend on Logitech, Moza, Thrustmaster, or other vendor SDKs.
Create IInputDeviceAdapter and device-specific adapters behind it.

Include:
- dead-zone configuration
- steering/pedal calibration
- inversion
- steering range
- sensitivity curves
- disconnect handling
- saved device profiles
- calibration UI
- raw and normalized telemetry

Do not implement force feedback yet, but leave an extension interface.
Add tests for mapping/calibration logic.
