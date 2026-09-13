using UnrealBuildTool;
using System.Collections.Generic;

public class UnitedTruckingSimulatorEditorTarget : TargetRules
{
    public UnitedTruckingSimulatorEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        ExtraModuleNames.AddRange(new string[] { "UTS", "UTSCore" });
    }
}
