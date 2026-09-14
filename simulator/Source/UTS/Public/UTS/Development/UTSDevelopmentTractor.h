#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "UTS/Input/IInputDeviceAdapter.h"
#include "UTS/Vehicle/SyntheticTractorMotion.h"
#include "UTSDevelopmentTractor.generated.h"

class UCameraComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

// Development-only synthetic visual pawn. This is a bounded prototype shell,
// not validated vehicle physics, not CDL training behavior, and not a trailer.
UCLASS()
class UTS_API AUTSDevelopmentTractor : public APawn
{
    GENERATED_BODY()

public:
    AUTSDevelopmentTractor();

    virtual void Tick(float DeltaSeconds) override;
    virtual void OnConstruction(const FTransform& Transform) override;

    // Tractor code consumes semantic controls only; no raw device reads live here.
    void SetControlFrame(const UTS::FSemanticControlFrame& Frame);

    UFUNCTION(BlueprintCallable, Category="UTS|Development")
    void ToggleCamera();

    void ResetPrototype();

    UFUNCTION(BlueprintPure, Category="UTS|Development")
    float GetPrototypeSpeedMps() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(VisibleAnywhere, Category="UTS|Development|Synthetic")
    TObjectPtr<USceneComponent> SyntheticRoot;

    UPROPERTY(VisibleAnywhere, Category="UTS|Development|Synthetic")
    TObjectPtr<UStaticMeshComponent> ChassisMesh;

    UPROPERTY(VisibleAnywhere, Category="UTS|Development|Synthetic")
    TObjectPtr<UStaticMeshComponent> HoodMesh;

    UPROPERTY(VisibleAnywhere, Category="UTS|Development|Synthetic")
    TObjectPtr<UStaticMeshComponent> CabMesh;

    UPROPERTY(VisibleAnywhere, Category="UTS|Development|Synthetic")
    TArray<TObjectPtr<UStaticMeshComponent>> WheelMeshes;

    UPROPERTY(VisibleAnywhere, Category="UTS|Development|Synthetic")
    TObjectPtr<UCameraComponent> OverheadCamera;

    UPROPERTY(VisibleAnywhere, Category="UTS|Development|Synthetic")
    TObjectPtr<UCameraComponent> CabCamera;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> SyntheticMaterial;

    UPROPERTY(EditAnywhere, Category="UTS|Development|Synthetic", meta=(ClampMin="1.0", Units="m"))
    float VisualLengthMetres = 6.0f;

    UPROPERTY(EditAnywhere, Category="UTS|Development|Synthetic", meta=(ClampMin="1.0", Units="m"))
    float VisualWidthMetres = 2.5f;

    UPROPERTY(EditAnywhere, Category="UTS|Development|Synthetic", meta=(ClampMin="0.2", Units="m"))
    float WheelRadiusMetres = 0.55f;

    UPROPERTY(EditAnywhere, Category="UTS|Development|Synthetic", meta=(ClampMin="0.1", Units="m"))
    float WheelWidthMetres = 0.45f;

    UPROPERTY(EditAnywhere, Category="UTS|Development|Synthetic", meta=(ClampMin="1.0", Units="m"))
    float OverheadCameraHeightMetres = 10.0f;

    UTS::FSemanticControlFrame LastControlFrame;
    UTS::FSyntheticTractorMotion Motion;
    FTransform InitialActorTransform;
    bool bUsingOverheadCamera = true;

    UStaticMeshComponent* CreateSyntheticMeshComponent(FName ComponentName, UStaticMesh* Mesh);
    void RefreshSyntheticVisuals();
    void ApplySyntheticPose();
    void SetActiveSyntheticCamera(bool bUseOverhead);
    bool IsSyntheticAttemptRunning() const;
};
