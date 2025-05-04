
#include "FishTailComponent.h"

#include "Engine/AssetManager.h"

UFishTailComponent::UFishTailComponent()
{
}

void UFishTailComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    ElapsedTime += DeltaTime;

    SetRelativeRotation(FRotator(0.f, FMath::Sin(ElapsedTime * PI * CurrentFrequency) * CurrentYaw, 0.f));

    // UE_LOG(ELogLevel::Display, TEXT("ElapsedTime: %f, Yaw: %f"), ElapsedTime, GetRelativeRotation().Yaw);
}

void UFishTailComponent::InitializeComponent()
{
    UStaticMeshComponent::InitializeComponent();
    
    SetStaticMesh(UAssetManager::Get().GetStaticMeshAsset(L"Contents/Fish/Fish_Back.obj"));
}
