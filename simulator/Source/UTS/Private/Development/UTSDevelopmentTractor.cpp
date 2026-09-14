#include "UTS/Development/UTSDevelopmentTractor.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "UTS/Session/UTSSessionSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogUTSDevelopmentTractor, Log, All);

namespace
{
    constexpr float TractorCentimetresPerMetre = 100.0f;

#if !UE_BUILD_SHIPPING
    void DisableTractorSyntheticCollision(UStaticMeshComponent* Component)
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

    void ConfigureTractorCubeCentimetres(UStaticMeshComponent* Component, const FVector& LocationCm, const FVector& DimensionsCm)
    {
        if (!Component)
        {
            return;
        }

        Component->SetRelativeLocation(LocationCm);
        Component->SetRelativeRotation(FRotator::ZeroRotator);
        Component->SetRelativeScale3D(DimensionsCm / TractorCentimetresPerMetre);
    }

    void ConfigureTractorWheel(UStaticMeshComponent* Component, const FVector& LocationCm, float RadiusCm, float WidthCm)
    {
        if (!Component)
        {
            return;
        }

        const float DiameterCm = RadiusCm * 2.0f;
        Component->SetRelativeLocation(LocationCm);
        Component->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));
        Component->SetRelativeScale3D(FVector(DiameterCm / TractorCentimetresPerMetre,
            DiameterCm / TractorCentimetresPerMetre,
            WidthCm / TractorCentimetresPerMetre));
    }

    void ApplyTractorSyntheticColor(UStaticMeshComponent* Component, UMaterialInterface* BaseMaterial, const FLinearColor& Color)
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

AUTSDevelopmentTractor::AUTSDevelopmentTractor()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorEnableCollision(false);

#if !UE_BUILD_SHIPPING
    SyntheticRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SyntheticTractorRoot"));
    SyntheticRoot->SetMobility(EComponentMobility::Movable);
    SetRootComponent(SyntheticRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    SyntheticMaterial = BasicShapeMaterial.Object;
    if (!CubeMesh.Succeeded())
    {
        UE_LOG(LogUTSDevelopmentTractor, Warning, TEXT("Missing synthetic tractor cube mesh: /Engine/BasicShapes/Cube.Cube"));
    }
    if (!CylinderMesh.Succeeded())
    {
        UE_LOG(LogUTSDevelopmentTractor, Warning, TEXT("Missing synthetic tractor cylinder mesh: /Engine/BasicShapes/Cylinder.Cylinder"));
    }
    if (!BasicShapeMaterial.Succeeded())
    {
        UE_LOG(LogUTSDevelopmentTractor, Warning, TEXT("Missing synthetic tractor material: /Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    }

    ChassisMesh = CreateSyntheticMeshComponent(TEXT("SyntheticChassis"), CubeMesh.Object);
    HoodMesh = CreateSyntheticMeshComponent(TEXT("SyntheticHood"), CubeMesh.Object);
    CabMesh = CreateSyntheticMeshComponent(TEXT("SyntheticCab"), CubeMesh.Object);

    static const TCHAR* WheelNames[] =
    {
        TEXT("SyntheticFrontLeftWheel"),
        TEXT("SyntheticFrontRightWheel"),
        TEXT("SyntheticRearLeftForwardWheel"),
        TEXT("SyntheticRearRightForwardWheel"),
        TEXT("SyntheticRearLeftAftWheel"),
        TEXT("SyntheticRearRightAftWheel")
    };

    WheelMeshes.Reserve(UE_ARRAY_COUNT(WheelNames));
    for (const TCHAR* WheelName : WheelNames)
    {
        WheelMeshes.Add(CreateSyntheticMeshComponent(FName(WheelName), CylinderMesh.Object));
    }

    OverheadCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("SyntheticOverheadCamera"));
    OverheadCamera->SetupAttachment(SyntheticRoot);
    OverheadCamera->SetMobility(EComponentMobility::Movable);
    OverheadCamera->bUsePawnControlRotation = false;

    CabCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("SyntheticCabCamera"));
    CabCamera->SetupAttachment(SyntheticRoot);
    CabCamera->SetMobility(EComponentMobility::Movable);
    CabCamera->bUsePawnControlRotation = false;

    RefreshSyntheticVisuals();
    SetActiveSyntheticCamera(true);
#endif
}

void AUTSDevelopmentTractor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

#if !UE_BUILD_SHIPPING
    RefreshSyntheticVisuals();
    SetActiveSyntheticCamera(bUsingOverheadCamera);
#endif
}

void AUTSDevelopmentTractor::BeginPlay()
{
    Super::BeginPlay();

#if !UE_BUILD_SHIPPING
    InitialActorTransform = GetActorTransform();

    ApplyTractorSyntheticColor(ChassisMesh, SyntheticMaterial, FLinearColor(0.08f, 0.08f, 0.09f));
    ApplyTractorSyntheticColor(HoodMesh, SyntheticMaterial, FLinearColor(0.05f, 0.18f, 0.65f));
    ApplyTractorSyntheticColor(CabMesh, SyntheticMaterial, FLinearColor(0.07f, 0.24f, 0.85f));
    for (UStaticMeshComponent* WheelMesh : WheelMeshes)
    {
        ApplyTractorSyntheticColor(WheelMesh, SyntheticMaterial, FLinearColor(0.01f, 0.01f, 0.01f));
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UUTSSessionSubsystem* Session = GameInstance->GetSubsystem<UUTSSessionSubsystem>())
        {
            Session->OnPrototypeResetRequested.AddUObject(this, &AUTSDevelopmentTractor::ResetPrototype);
        }
    }

    SetActiveSyntheticCamera(bUsingOverheadCamera);
    ApplySyntheticPose();
#endif
}

void AUTSDevelopmentTractor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if !UE_BUILD_SHIPPING
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UUTSSessionSubsystem* Session = GameInstance->GetSubsystem<UUTSSessionSubsystem>())
        {
            Session->OnPrototypeResetRequested.RemoveAll(this);
        }
    }
#endif

    Super::EndPlay(EndPlayReason);
}

void AUTSDevelopmentTractor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

#if !UE_BUILD_SHIPPING
    if (DeltaSeconds <= 0.0f)
    {
        return;
    }

    const bool bAttemptRunning = IsSyntheticAttemptRunning();
    if (!bAttemptRunning)
    {
        LastControlFrame = UTS::FSemanticControlFrame{};
    }

    Motion.Step(LastControlFrame, static_cast<double>(DeltaSeconds), bAttemptRunning);
    ApplySyntheticPose();
#endif
}

void AUTSDevelopmentTractor::SetControlFrame(const UTS::FSemanticControlFrame& Frame)
{
#if !UE_BUILD_SHIPPING
    LastControlFrame = Frame;
#endif
}

void AUTSDevelopmentTractor::ToggleCamera()
{
#if !UE_BUILD_SHIPPING
    SetActiveSyntheticCamera(!bUsingOverheadCamera);
#endif
}

void AUTSDevelopmentTractor::ResetPrototype()
{
#if !UE_BUILD_SHIPPING
    LastControlFrame = UTS::FSemanticControlFrame{};
    Motion.Reset();
    ApplySyntheticPose();
#endif
}

float AUTSDevelopmentTractor::GetPrototypeSpeedMps() const
{
#if !UE_BUILD_SHIPPING
    return static_cast<float>(Motion.GetPose().SpeedMps);
#else
    return 0.0f;
#endif
}

UStaticMeshComponent* AUTSDevelopmentTractor::CreateSyntheticMeshComponent(FName ComponentName, UStaticMesh* Mesh)
{
#if !UE_BUILD_SHIPPING
    UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(ComponentName);
    Component->SetupAttachment(SyntheticRoot);
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetStaticMesh(Mesh);
    DisableTractorSyntheticCollision(Component);
    return Component;
#else
    return nullptr;
#endif
}

void AUTSDevelopmentTractor::RefreshSyntheticVisuals()
{
#if !UE_BUILD_SHIPPING
    const float LengthCm = VisualLengthMetres * TractorCentimetresPerMetre;
    const float WidthCm = VisualWidthMetres * TractorCentimetresPerMetre;
    const float WheelRadiusCm = WheelRadiusMetres * TractorCentimetresPerMetre;
    const float WheelWidthCm = WheelWidthMetres * TractorCentimetresPerMetre;

    ConfigureTractorCubeCentimetres(ChassisMesh,
        FVector(-0.35f * LengthCm, 0.0f, WheelRadiusCm + 32.0f),
        FVector(0.58f * LengthCm, 0.82f * WidthCm, 64.0f));

    ConfigureTractorCubeCentimetres(HoodMesh,
        FVector(0.22f * LengthCm, 0.0f, WheelRadiusCm + 88.0f),
        FVector(0.36f * LengthCm, 0.72f * WidthCm, 96.0f));

    // Cab camera sits just ahead of the cab windshield so it does not start
    // inside an opaque cube in this synthetic shell.
    ConfigureTractorCubeCentimetres(CabMesh,
        FVector(-0.12f * LengthCm, 0.0f, WheelRadiusCm + 150.0f),
        FVector(0.28f * LengthCm, 0.74f * WidthCm, 176.0f));

    const float WheelY = (WidthCm * 0.5f) + (WheelWidthCm * 0.5f);
    const float FrontX = 0.34f * LengthCm;
    const float RearForwardX = -0.24f * LengthCm;
    const float RearAftX = -0.38f * LengthCm;
    const FVector WheelLocations[] =
    {
        FVector(FrontX, -WheelY, WheelRadiusCm),
        FVector(FrontX, WheelY, WheelRadiusCm),
        FVector(RearForwardX, -WheelY, WheelRadiusCm),
        FVector(RearForwardX, WheelY, WheelRadiusCm),
        FVector(RearAftX, -WheelY, WheelRadiusCm),
        FVector(RearAftX, WheelY, WheelRadiusCm)
    };

    for (int32 Index = 0; Index < WheelMeshes.Num() && Index < UE_ARRAY_COUNT(WheelLocations); ++Index)
    {
        ConfigureTractorWheel(WheelMeshes[Index], WheelLocations[Index], WheelRadiusCm, WheelWidthCm);
    }

    if (OverheadCamera)
    {
        OverheadCamera->SetRelativeLocation(FVector(-0.35f * LengthCm, 0.0f,
            OverheadCameraHeightMetres * TractorCentimetresPerMetre));
        OverheadCamera->SetRelativeRotation(FRotator(-65.0f, 0.0f, 0.0f));
    }

    if (CabCamera)
    {
        CabCamera->SetRelativeLocation(FVector(0.13f * LengthCm, 0.0f, WheelRadiusCm + 210.0f));
        CabCamera->SetRelativeRotation(FRotator(-5.0f, 0.0f, 0.0f));
    }
#endif
}

void AUTSDevelopmentTractor::ApplySyntheticPose()
{
#if !UE_BUILD_SHIPPING
    const UTS::FSyntheticTractorPose& Pose = Motion.GetPose();
    const FQuat InitialRotation = InitialActorTransform.GetRotation();
    const FVector InitialLocation = InitialActorTransform.GetLocation();
    const FVector LocalOffsetCm(
        static_cast<float>(Pose.XMetres * TractorCentimetresPerMetre),
        static_cast<float>(Pose.YMetres * TractorCentimetresPerMetre),
        0.0f);

    const FVector WorldLocation = InitialLocation + InitialRotation.RotateVector(LocalOffsetCm);
    const FQuat WorldRotation = InitialRotation * FQuat(FVector::UpVector, static_cast<float>(Pose.YawRadians));
    SetActorLocationAndRotation(WorldLocation, WorldRotation, false, nullptr, ETeleportType::TeleportPhysics);
#endif
}

void AUTSDevelopmentTractor::SetActiveSyntheticCamera(bool bUseOverhead)
{
#if !UE_BUILD_SHIPPING
    bUsingOverheadCamera = bUseOverhead;

    if (OverheadCamera)
    {
        OverheadCamera->SetActive(bUsingOverheadCamera);
        OverheadCamera->SetVisibility(bUsingOverheadCamera, true);
    }

    if (CabCamera)
    {
        CabCamera->SetActive(!bUsingOverheadCamera);
        CabCamera->SetVisibility(!bUsingOverheadCamera, true);
    }
#endif
}

bool AUTSDevelopmentTractor::IsSyntheticAttemptRunning() const
{
#if !UE_BUILD_SHIPPING
    const UGameInstance* GameInstance = GetGameInstance();
    const UUTSSessionSubsystem* Session = GameInstance ? GameInstance->GetSubsystem<UUTSSessionSubsystem>() : nullptr;
    return Session && Session->IsAttemptRunning();
#else
    return false;
#endif
}
