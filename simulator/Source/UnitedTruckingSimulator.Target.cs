using UnrealBuildTool;
using System.Collections.Generic;

public class UnitedTruckingSimulatorTarget : TargetRules
{
    public UnitedTruckingSimulatorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        CppStandard = CppStandardVersion.Cpp20;
        ExtraModuleNames.AddRange(new string[] { "UTS", "UTSCore" });
    }
}
