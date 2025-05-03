
#include "ItemActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/AssetManager.h"

AItemActor::AItemActor()
{
}

void AItemActor::PostSpawnInitialize()
{
    AActor::PostSpawnInitialize();

    SphereComponent = AddComponent<USphereComponent>(FName("SphereComponent_0"));
    SetRootComponent(SphereComponent);

    MeshComponent = AddComponent<UStaticMeshComponent>(FName("MeshComponent_0"));
    MeshComponent->SetStaticMesh(UAssetManager::Get().GetStaticMesh(L"Contents/Coin2/Coin2.obj"));
    MeshComponent->SetupAttachment(RootComponent);
}

void AItemActor::BeginPlay()
{
    AActor::BeginPlay();

    SetHidden(false);

    OnActorBeginOverlap.AddLambda(
        [this](AActor* OverlappedActor, AActor* OtherActor)
        {
            ActorBeginOverlap(OverlappedActor, OtherActor);
        }
    );
}

void AItemActor::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);

    ElapsedTime += DeltaTime;

    if (UStaticMeshComponent* MeshComp = GetComponentByClass<UStaticMeshComponent>())
    {
        MeshComp->SetRelativeRotation(FRotator(0.f, ElapsedTime * Speed, 0.f));
        MeshComp->SetRelativeLocation(FVector(0.f, 0.f, FMath::Sin(ElapsedTime * FloatingFrequency) * FloatingHeight));       
    }
}

void AItemActor::Reset()
{
    SetHidden(false);
}

void AItemActor::ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
    
}
