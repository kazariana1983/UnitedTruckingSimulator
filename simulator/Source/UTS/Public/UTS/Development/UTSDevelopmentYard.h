#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UTSDevelopmentYard.generated.h"

class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

// Development-only synthetic paved yard. Geometry is generated from engine
// primitives at runtime; it is not an approved CDL yard layout or scoring map.
UCLASS()
class UTS_API AUTSDevelopmentYard : public AActor
{
    GENERATED_BODY()

public:
    AUTSDevelopmentYard();

    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    virtual void BeginPlay() override;

private:
    static constexpr int32 MaxSyntheticCones = 40;

    UPROPERTY(VisibleAnywhere, Category="UTS|Development|Synthetic")
    TObjectPtr<USceneComponent> SyntheticRoot;

    UPROPERTY(VisibleAnywhere, Category="UTS|Development|Synthetic")
    TObjectPtr<UStaticMeshComponent> DeckMesh;

    UPROPERTY(VisibleAnywhere, Category="UTS|Development|Synthetic")
    TObjectPtr<UStaticMeshComponent> NorthBoundaryMesh;

    UPROPERTY(VisibleAnywhere, Category="UTS|Development|Synthetic")
    TObjectPtr<UStaticMeshComponent> SouthBoundaryMesh;

    UPROPERTY(VisibleAnywhere, Category="UTS|Development|Synthetic")
    TObjectPtr<UStaticMeshComponent> EastBoundaryMesh;

    UPROPERTY(VisibleAnywhere, Category="UTS|Development|Synthetic")
    TObjectPtr<UStaticMeshComponent> WestBoundaryMesh;

    UPROPERTY(VisibleAnywhere, Category="UTS|Development|Synthetic")
    TObjectPtr<UStaticMeshComponent> LaneLeftBoundaryMesh;

    UPROPERTY(VisibleAnywhere, Category="UTS|Development|Synthetic")
    TObjectPtr<UStaticMeshComponent> LaneRightBoundaryMesh;

    UPROPERTY(VisibleAnywhere, Category="UTS|Development|Synthetic")
    TObjectPtr<UStaticMeshComponent> LaneBackStopMesh;

    UPROPERTY(VisibleAnywhere, Category="UTS|Development|Synthetic")
    TArray<TObjectPtr<UStaticMeshComponent>> ConeMeshes;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> SyntheticMaterial;

    UPROPERTY(EditAnywhere, Category="UTS|Development|Synthetic", meta=(ClampMin="5.0", Units="m"))
    float YardHalfExtentMetres = 50.0f;

    UPROPERTY(EditAnywhere, Category="UTS|Development|Synthetic", meta=(ClampMin="5.0", Units="m"))
    float BackingLaneLengthMetres = 32.0f;

    UPROPERTY(EditAnywhere, Category="UTS|Development|Synthetic", meta=(ClampMin="2.0", Units="m"))
    float BackingLaneWidthMetres = 4.5f;

    UPROPERTY(EditAnywhere, Category="UTS|Development|Synthetic", meta=(ClampMin="1.0", Units="m"))
    float ConeSpacingMetres = 4.0f;

    UPROPERTY(EditAnywhere, Category="UTS|Development|Synthetic", meta=(ClampMin="5.0", Units="cm"))
    float DeckThicknessCentimetres = 20.0f;

    UPROPERTY(EditAnywhere, Category="UTS|Development|Synthetic", meta=(ClampMin="5.0", Units="cm"))
    float BoundaryWidthCentimetres = 20.0f;

    UPROPERTY(EditAnywhere, Category="UTS|Development|Synthetic", meta=(ClampMin="5.0", Units="cm"))
    float BoundaryHeightCentimetres = 20.0f;

    UPROPERTY(EditAnywhere, Category="UTS|Development|Synthetic", meta=(ClampMin="10.0", Units="cm"))
    float ConeHeightCentimetres = 75.0f;

    UStaticMeshComponent* CreateSyntheticMeshComponent(FName ComponentName, UStaticMesh* Mesh);
    void RefreshSyntheticLayout();
};
