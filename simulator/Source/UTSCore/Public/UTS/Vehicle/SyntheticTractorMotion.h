#pragma once
#include <algorithm>
#include <cmath>
#include "UTS/Input/IInputDeviceAdapter.h"

namespace UTS
{
// Developer toy values only: not vehicle specifications or CDL yard dimensions.
struct FSyntheticTractorSettings
{
    double WheelbaseMetres = 4.0;
    double MaximumSpeedMps = 5.0;
    double AccelerationMps2 = 3.0;
    double BrakeDecelerationMps2 = 6.0;
    double MaximumSteeringRadians = 0.5;
    double YardHalfExtentMetres = 40.0;
};
struct FSyntheticTractorPose
{
    double XMetres = 0.0, YMetres = 0.0, YawRadians = 0.0, SpeedMps = 0.0;
};
class FSyntheticTractorMotion
{
public:
    explicit FSyntheticTractorMotion(FSyntheticTractorSettings Settings = {}) : Settings_(Settings) {}
    const FSyntheticTractorPose& GetPose() const { return Pose_; }
    void Reset() { Pose_ = {}; }
    void Step(const FSemanticControlFrame& Input, double DeltaSeconds, bool Enabled)
    {
        if (!Enabled || !ValidSettings() || !std::isfinite(DeltaSeconds) || DeltaSeconds <= 0.0 ||
            !std::isfinite(Input.SteeringNormalized) || !std::isfinite(Input.ThrottleNormalized) ||
            !std::isfinite(Input.BrakeNormalized))
        { Pose_.SpeedMps = 0.0; return; }
        // Drop excess time on a stalled frame: never teleport across the yard.
        double Remaining = std::min(DeltaSeconds, 0.1);
        while (Remaining > 1e-9)
        {
            const double Dt = std::min(Remaining, 1.0 / 120.0);
            Remaining -= Dt;
            const int Gear = Input.GearSelect.value_or(0);
            const double Direction = Gear == 1 ? 1.0 : Gear == -1 ? -1.0 : 0.0;
            const double Throttle = std::clamp<double>(Input.ThrottleNormalized, 0, 1);
            const double Brake = std::clamp<double>(Input.BrakeNormalized, 0, 1);
            if (Brake > 0.0 || Input.bParkingBrakeEngaged || Direction == 0.0 || Throttle == 0.0 ||
                Pose_.SpeedMps * Direction < 0.0)
            {
                const double Strength = Input.bParkingBrakeEngaged ? 1.0 : std::max(Brake, 0.5);
                const double Speed = std::max(0.0, std::abs(Pose_.SpeedMps) - Settings_.BrakeDecelerationMps2 * Strength * Dt);
                Pose_.SpeedMps = std::copysign(Speed, Pose_.SpeedMps);
            }
            else
                Pose_.SpeedMps = std::clamp(Pose_.SpeedMps + Direction * Throttle * Settings_.AccelerationMps2 * Dt,
                    -Settings_.MaximumSpeedMps, Settings_.MaximumSpeedMps);
            const double Steer = std::clamp<double>(Input.SteeringNormalized, -1, 1) * Settings_.MaximumSteeringRadians;
            const double NextYaw = Pose_.YawRadians + Pose_.SpeedMps / Settings_.WheelbaseMetres * std::tan(Steer) * Dt;
            const double X = Pose_.XMetres + Pose_.SpeedMps * std::cos(NextYaw) * Dt;
            const double Y = Pose_.YMetres + Pose_.SpeedMps * std::sin(NextYaw) * Dt;
            // Containment only, not collision detection or a scored boundary event.
            if (std::abs(X) > Settings_.YardHalfExtentMetres || std::abs(Y) > Settings_.YardHalfExtentMetres)
                Pose_.SpeedMps = 0.0;
            else { Pose_.XMetres = X; Pose_.YMetres = Y; Pose_.YawRadians = std::remainder(NextYaw, 2.0 * 3.141592653589793); }
        }
    }
private:
    bool ValidSettings() const
    {
        const double Values[] = {Settings_.WheelbaseMetres, Settings_.MaximumSpeedMps, Settings_.AccelerationMps2,
            Settings_.BrakeDecelerationMps2, Settings_.MaximumSteeringRadians, Settings_.YardHalfExtentMetres};
        for (double Value : Values) if (!std::isfinite(Value) || Value <= 0.0) return false;
        return Settings_.MaximumSteeringRadians < 1.5;
    }
    FSyntheticTractorSettings Settings_;
    FSyntheticTractorPose Pose_;
};

// Keyboard-specific mapping stays outside vehicle simulation.
struct FSyntheticKeyboardAdapter
{
    static FSemanticControlFrame Normalize(bool Forward, bool Reverse, bool Left, bool Right, bool Brake)
    {
        FSemanticControlFrame Frame;
        Frame.SteeringNormalized = static_cast<float>(Right) - static_cast<float>(Left);
        Frame.GearSelect = Forward != Reverse ? (Forward ? 1 : -1) : 0;
        Frame.ThrottleNormalized = Forward != Reverse ? 1.0f : 0.0f;
        Frame.BrakeNormalized = Brake || (Forward && Reverse) ? 1.0f : 0.0f;
        return Frame;
    }
};
}
