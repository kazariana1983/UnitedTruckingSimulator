#include "UTS/Development/UTSDevelopmentYard.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogUTSDevelopmentYard, Log, All);

namespace
{
    constexpr float YardCentimetresPerMetre = 100.0f;

#if !UE_BUILD_SHIPPING
    void DisableYardSyntheticCollision(UStaticMeshComponent* Component)
    {
        if (!Component)
        {
            return;
        }

        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->SetCanEverAffectNavigation(false);
        Component->CanCharacterStepUpOn = ECB_No;
    }

    void ConfigureYardCubeCentimetres(UStaticMeshComponent* Component, const FVector& LocationCm, const FVector& DimensionsCm)
    {
        if (!Component)
        {
            return;
        }

        Component->SetHiddenInGame(false);
        Component->SetVisibility(true, true);
        Component->SetRelativeLocation(LocationCm);
        Component->SetRelativeRotation(FRotator::ZeroRotator);
        Component->SetRelativeScale3D(DimensionsCm / YardCentimetresPerMetre);
    }

    void HideYardSyntheticComponent(UStaticMeshComponent* Component)
    {
        if (!Component)
        {
            return;
        }

        Component->SetHiddenInGame(true);
        Component->SetVisibility(false, true);
    }

    void ApplyYardSyntheticColor(UStaticMeshComponent* Component, UMaterialInterface* BaseMaterial, const FLinearColor& Color)
    {
        if (!Component || !BaseMaterial)
        {
            return;
        }

        UMaterialInstanceDynamic* DynamicMaterial = Component->CreateDynamicMaterialInstance(0, BaseMaterial);
        if (DynamicMaterial)
        {
            DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
            DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
        }
    }
#endif
}

AUTSDevelopmentYard::AUTSDevelopmentYard()
{
    PrimaryActorTick.bCanEverTick = false;
    SetActorEnableCollision(false);

#if !UE_BUILD_SHIPPING
    SyntheticRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SyntheticYardRoot"));
    SetRootComponent(SyntheticRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    SyntheticMaterial = BasicShapeMaterial.Object;
    if (!CubeMesh.Succeeded())
    {
        UE_LOG(LogUTSDevelopmentYard, Warning, TEXT("Missing synthetic yard cube mesh: /Engine/BasicShapes/Cube.Cube"));
    }
    if (!ConeMesh.Succeeded())
    {
        UE_LOG(LogUTSDevelopmentYard, Warning, TEXT("Missing synthetic yard cone mesh: /Engine/BasicShapes/Cone.Cone"));
    }
    if (!BasicShapeMaterial.Succeeded())
    {
        UE_LOG(LogUTSDevelopmentYard, Warning, TEXT("Missing synthetic yard material: /Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    }

    DeckMesh = CreateSyntheticMeshComponent(TEXT("SyntheticPavedDeck"), CubeMesh.Object);
    NorthBoundaryMesh = CreateSyntheticMeshComponent(TEXT("SyntheticNorthBoundary"), CubeMesh.Object);
    SouthBoundaryMesh = CreateSyntheticMeshComponent(TEXT("SyntheticSouthBoundary"), CubeMesh.Object);
    EastBoundaryMesh = CreateSyntheticMeshComponent(TEXT("SyntheticEastBoundary"), CubeMesh.Object);
    WestBoundaryMesh = CreateSyntheticMeshComponent(TEXT("SyntheticWestBoundary"), CubeMesh.Object);
    LaneLeftBoundaryMesh = CreateSyntheticMeshComponent(TEXT("SyntheticBackingLaneLeftBoundary"), CubeMesh.Object);
    LaneRightBoundaryMesh = CreateSyntheticMeshComponent(TEXT("SyntheticBackingLaneRightBoundary"), CubeMesh.Object);
    LaneBackStopMesh = CreateSyntheticMeshComponent(TEXT("SyntheticBackingLaneBackStop"), CubeMesh.Object);

    ConeMeshes.Reserve(MaxSyntheticCones);
    for (int32 Index = 0; Index < MaxSyntheticCones; ++Index)
    {
        ConeMeshes.Add(CreateSyntheticMeshComponent(*FString::Printf(TEXT("SyntheticBackingCone_%02d"), Index), ConeMesh.Object));
    }

    RefreshSyntheticLayout();
#endif
}

void AUTSDevelopmentYard::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

#if !UE_BUILD_SHIPPING
    RefreshSyntheticLayout();
#endif
}

void AUTSDevelopmentYard::BeginPlay()
{
    Super::BeginPlay();

#if !UE_BUILD_SHIPPING
    ApplyYardSyntheticColor(DeckMesh, SyntheticMaterial, FLinearColor(0.18f, 0.18f, 0.18f));
    ApplyYardSyntheticColor(NorthBoundaryMesh, SyntheticMaterial, FLinearColor(0.9f, 0.9f, 0.9f));
    ApplyYardSyntheticColor(SouthBoundaryMesh, SyntheticMaterial, FLinearColor(0.9f, 0.9f, 0.9f));
    ApplyYardSyntheticColor(EastBoundaryMesh, SyntheticMaterial, FLinearColor(0.9f, 0.9f, 0.9f));
    ApplyYardSyntheticColor(WestBoundaryMesh, SyntheticMaterial, FLinearColor(0.9f, 0.9f, 0.9f));
    ApplyYardSyntheticColor(LaneLeftBoundaryMesh, SyntheticMaterial, FLinearColor(1.0f, 0.85f, 0.05f));
    ApplyYardSyntheticColor(LaneRightBoundaryMesh, SyntheticMaterial, FLinearColor(1.0f, 0.85f, 0.05f));
    ApplyYardSyntheticColor(LaneBackStopMesh, SyntheticMaterial, FLinearColor(1.0f, 0.85f, 0.05f));

    for (UStaticMeshComponent* ConeMesh : ConeMeshes)
    {
        ApplyYardSyntheticColor(ConeMesh, SyntheticMaterial, FLinearColor(1.0f, 0.32f, 0.02f));
    }
#endif
}

UStaticMeshComponent* AUTSDevelopmentYard::CreateSyntheticMeshComponent(FName ComponentName, UStaticMesh* Mesh)
{
#if !UE_BUILD_SHIPPING
    UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(ComponentName);
    Component->SetupAttachment(SyntheticRoot);
    Component->SetStaticMesh(Mesh);
    DisableYardSyntheticCollision(Component);
    return Component;
#else
    return nullptr;
#endif
}

void AUTSDevelopmentYard::RefreshSyntheticLayout()
{
#if !UE_BUILD_SHIPPING
    const float HalfExtentCm = YardHalfExtentMetres * YardCentimetresPerMetre;
    const float FullExtentCm = HalfExtentCm * 2.0f;
    const float BoundaryWidthCm = BoundaryWidthCentimetres;
    const float BoundaryHeightCm = BoundaryHeightCentimetres;
    const float BoundaryZCm = BoundaryHeightCm * 0.5f;

    // Deck top is local Z=0 so the tractor can be spawned at the same origin
    // with wheels above zero in this synthetic shell.
    ConfigureYardCubeCentimetres(DeckMesh,
        FVector(0.0f, 0.0f, -DeckThicknessCentimetres * 0.5f),
        FVector(FullExtentCm, FullExtentCm, DeckThicknessCentimetres));

    ConfigureYardCubeCentimetres(NorthBoundaryMesh,
        FVector(0.0f, HalfExtentCm, BoundaryZCm),
        FVector(FullExtentCm, BoundaryWidthCm, BoundaryHeightCm));
    ConfigureYardCubeCentimetres(SouthBoundaryMesh,
        FVector(0.0f, -HalfExtentCm, BoundaryZCm),
        FVector(FullExtentCm, BoundaryWidthCm, BoundaryHeightCm));
    ConfigureYardCubeCentimetres(EastBoundaryMesh,
        FVector(HalfExtentCm, 0.0f, BoundaryZCm),
        FVector(BoundaryWidthCm, FullExtentCm, BoundaryHeightCm));
    ConfigureYardCubeCentimetres(WestBoundaryMesh,
        FVector(-HalfExtentCm, 0.0f, BoundaryZCm),
        FVector(BoundaryWidthCm, FullExtentCm, BoundaryHeightCm));

    const float LaneLengthCm = FMath::Clamp(BackingLaneLengthMetres * YardCentimetresPerMetre,
        YardCentimetresPerMetre, FullExtentCm - BoundaryWidthCm * 2.0f);
    const float LaneWidthCm = FMath::Clamp(BackingLaneWidthMetres * YardCentimetresPerMetre,
        YardCentimetresPerMetre, FullExtentCm - BoundaryWidthCm * 2.0f);
    const float LaneCenterXCm = -0.5f * LaneLengthCm;
    const float LaneSideYCm = LaneWidthCm * 0.5f;
    const float LaneStripeZCm = 3.0f;
    const float LaneStripeHeightCm = 6.0f;

    ConfigureYardCubeCentimetres(LaneLeftBoundaryMesh,
        FVector(LaneCenterXCm, -LaneSideYCm, LaneStripeZCm),
        FVector(LaneLengthCm, 12.0f, LaneStripeHeightCm));
    ConfigureYardCubeCentimetres(LaneRightBoundaryMesh,
        FVector(LaneCenterXCm, LaneSideYCm, LaneStripeZCm),
        FVector(LaneLengthCm, 12.0f, LaneStripeHeightCm));
    ConfigureYardCubeCentimetres(LaneBackStopMesh,
        FVector(-LaneLengthCm, 0.0f, LaneStripeZCm),
        FVector(12.0f, LaneWidthCm, LaneStripeHeightCm));

    const float ConeSpacingCm = FMath::Max(ConeSpacingMetres * YardCentimetresPerMetre, 100.0f);
    const float ConeScaleXY = FMath::Max(ConeHeightCentimetres * 0.45f, 20.0f) / YardCentimetresPerMetre;
    const float ConeScaleZ = ConeHeightCentimetres / YardCentimetresPerMetre;
    const float ConeZCm = ConeHeightCentimetres * 0.5f;
    int32 ConeIndex = 0;

    for (float X = 0.0f; X >= -LaneLengthCm && ConeIndex + 1 < ConeMeshes.Num(); X -= ConeSpacingCm)
    {
        UStaticMeshComponent* LeftCone = ConeMeshes[ConeIndex++];
        UStaticMeshComponent* RightCone = ConeMeshes[ConeIndex++];

        if (LeftCone)
        {
            LeftCone->SetHiddenInGame(false);
            LeftCone->SetVisibility(true, true);
            LeftCone->SetRelativeLocation(FVector(X, -LaneSideYCm, ConeZCm));
            LeftCone->SetRelativeRotation(FRotator::ZeroRotator);
            LeftCone->SetRelativeScale3D(FVector(ConeScaleXY, ConeScaleXY, ConeScaleZ));
        }

        if (RightCone)
        {
            RightCone->SetHiddenInGame(false);
            RightCone->SetVisibility(true, true);
            RightCone->SetRelativeLocation(FVector(X, LaneSideYCm, ConeZCm));
            RightCone->SetRelativeRotation(FRotator::ZeroRotator);
            RightCone->SetRelativeScale3D(FVector(ConeScaleXY, ConeScaleXY, ConeScaleZ));
        }
    }

    while (ConeIndex < ConeMeshes.Num())
    {
        HideYardSyntheticComponent(ConeMeshes[ConeIndex++]);
    }
#endif
}
