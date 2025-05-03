
#include "FishBodyComponent.h"

#include "Engine/AssetManager.h"

UFishBodyComponent::UFishBodyComponent()
{
}

void UFishBodyComponent::TickComponent(float DeltaTime)
{
    UStaticMeshComponent::TickComponent(DeltaTime);
}

void UFishBodyComponent::InitializeComponent()
{
    UStaticMeshComponent::InitializeComponent();

    SetStaticMesh(UAssetManager::Get().GetStaticMesh(L"Contents/Fish/Fish_Front.obj"));
    SetRelativeLocation(FVector(-0.5f, 0.f, 0.f));
    SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
}
