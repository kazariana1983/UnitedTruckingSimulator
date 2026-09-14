using UnrealBuildTool;
using System.Collections.Generic;

public class UnitedTruckingSimulatorTarget : TargetRules
{
    public UnitedTruckingSimulatorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        ExtraModuleNames.AddRange(new string[] { "UTS", "UTSCore" });
    }
}
