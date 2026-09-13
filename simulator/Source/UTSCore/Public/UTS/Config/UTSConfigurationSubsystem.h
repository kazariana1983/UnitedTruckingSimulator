#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "UTS/Config/ConfigurationStore.h"
#include "UTS/Config/IConfigurationProvider.h"

#include "UTSConfigurationSubsystem.generated.h"

UCLASS()
class UTSCORE_API UUTSConfigurationSubsystem final : public UGameInstanceSubsystem, public UTS::IConfigurationProvider
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "UTS|Configuration")
    bool ReloadConfigurationProfiles();

    UFUNCTION(BlueprintCallable, Category = "UTS|Configuration")
    bool IsValidatedLaunchAvailableForExercise(const FString& ExerciseProfileId);

    virtual UTS::FConfigLoadResult LoadProfile(
        UTS::EProfileType Type,
        const UTS::FProfileId& ProfileId,
        uint32_t RequestedVersion) override;

    virtual UTS::FValidatedConfigSet ValidateForLaunch(const UTS::FProfileId& ExerciseId) override;

    virtual UTS::EApprovalState GetApprovalState(
        const UTS::FProfileId& ProfileId,
        uint32_t Version) const override;

    const UTS::FFileConfigurationProvider& GetProviderForTests() const { return Provider; }

private:
    UTS::FFileConfigurationProvider Provider;
    FString LastProfileRoot;
};
