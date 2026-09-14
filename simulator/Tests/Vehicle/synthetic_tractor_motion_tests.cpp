#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include "UTS/Vehicle/SyntheticTractorMotion.h"
using namespace UTS;
void Advance(FSyntheticTractorMotion& M, FSemanticControlFrame F, int Count = 120)
{ for (int I=0; I<Count; ++I) M.Step(F, 1.0/60.0, true); }
int main()
{
    const auto Forward = FSyntheticKeyboardAdapter::Normalize(true,false,false,false,false);
    const auto Reverse = FSyntheticKeyboardAdapter::Normalize(false,true,false,false,false);
    const auto Brake = FSyntheticKeyboardAdapter::Normalize(true,false,false,false,true);
    const auto Both = FSyntheticKeyboardAdapter::Normalize(true,true,true,true,false);
    assert(Both.GearSelect == 0 && Both.ThrottleNormalized == 0 && Both.BrakeNormalized == 1 && Both.SteeringNormalized == 0);
    FSyntheticTractorMotion M;
    M.Step(Forward, 1.0/60.0, false); assert(M.GetPose().XMetres == 0);
    Advance(M, Forward); assert(M.GetPose().XMetres > 0 && M.GetPose().YMetres == 0 && M.GetPose().SpeedMps <= 5);
    const auto BeforePause = M.GetPose();
    M.Step(Forward, 0.1, false); assert(M.GetPose().XMetres == BeforePause.XMetres && M.GetPose().SpeedMps == 0);
    Advance(M, Forward); Advance(M, Brake); assert(M.GetPose().SpeedMps == 0);
    M.Reset(); Advance(M, Reverse); assert(M.GetPose().XMetres < 0);
    auto Turn = Forward; Turn.SteeringNormalized = 1;
    M.Reset(); Advance(M, Turn); assert(M.GetPose().YMetres > 0 && M.GetPose().YawRadians > 0);
    Turn.GearSelect = -1; M.Reset(); Advance(M, Turn); assert(M.GetPose().YawRadians < 0);
    M.Reset(); Advance(M, Forward, 3000); assert(std::abs(M.GetPose().XMetres) <= 40 && std::abs(M.GetPose().YMetres) <= 40);
    M.Reset(); assert(M.GetPose().XMetres == 0 && M.GetPose().YawRadians == 0 && M.GetPose().SpeedMps == 0);
    M.Step(Forward, 1000.0, true); assert(M.GetPose().XMetres < 0.1);
    auto Invalid = Forward; Invalid.SteeringNormalized = std::numeric_limits<float>::quiet_NaN();
    const double LastX=M.GetPose().XMetres; M.Step(Invalid, .1, true); assert(M.GetPose().XMetres == LastX && M.GetPose().SpeedMps == 0);
    M.Step(Forward, std::numeric_limits<double>::infinity(), true); assert(M.GetPose().XMetres == LastX);
    FSyntheticTractorSettings InvalidSettings; InvalidSettings.WheelbaseMetres = 0;
    FSyntheticTractorMotion InvalidMotion(InvalidSettings); Advance(InvalidMotion, Forward); assert(InvalidMotion.GetPose().XMetres == 0);
    FSyntheticTractorMotion A,B; Advance(A, Turn); Advance(B, Turn);
    assert(A.GetPose().XMetres == B.GetPose().XMetres && A.GetPose().YawRadians == B.GetPose().YawRadians);
    std::cout << "Synthetic tractor motion tests passed.\n";
}
