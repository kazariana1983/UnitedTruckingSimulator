using UnrealBuildTool;

public class UTSCore : ModuleRules
{
    public UTSCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bEnableExceptions = true;

        // The standard C++ loader reads loose files, not Unreal PAK entries.
        // These synthetic/draft fixtures are development-only.
        if (Target.Configuration != UnrealTargetConfiguration.Shipping)
        {
            RuntimeDependencies.Add("$(ProjectDir)/Config/ProfileSets/...", StagedFileType.NonUFS);
        }

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Projects"
        });
    }
}
