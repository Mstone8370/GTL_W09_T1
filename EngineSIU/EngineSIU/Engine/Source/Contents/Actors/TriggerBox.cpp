#include "TriggerBox.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/AssetManager.h"

ATriggerBox::ATriggerBox()
{
    BoxComponent = AddComponent<UBoxComponent>(FName("BoxComponent_0"));
    RootComponent = BoxComponent;

    MeshComponent = AddComponent<UStaticMeshComponent>(FName("MeshComponent_0"));
    MeshComponent->SetStaticMesh(UAssetManager::Get().GetStaticMeshAsset(L"Contents/FryBasket/FryBasket.obj"));
    MeshComponent->SetupAttachment(BoxComponent);
}

void ATriggerBox::PostSpawnInitialize()
{
    AActor::PostSpawnInitialize();
}

void ATriggerBox::BeginPlay()
{
    AActor::BeginPlay();
}

void ATriggerBox::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);
}

void ATriggerBox::ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
}
