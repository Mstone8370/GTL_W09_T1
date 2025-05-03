
#include "GoalPlatformActor.h"

#include "Engine/AssetManager.h"

AGoalPlatformActor::AGoalPlatformActor()
{
    BoxComponent = AddComponent<UBoxComponent>(FName("BoxComponent_0"));
    RootComponent = BoxComponent;

    MeshComponent = AddComponent<UStaticMeshComponent>(FName("MeshComponent_0"));
    MeshComponent->SetStaticMesh(UAssetManager::Get().GetStaticMesh(L"Contents/FryBasket/FryBasket.obj"));
    MeshComponent->SetupAttachment(BoxComponent);
}
