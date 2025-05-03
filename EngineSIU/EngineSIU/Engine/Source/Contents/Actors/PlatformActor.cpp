
#include "PlatformActor.h"

#include "Engine/AssetManager.h"

APlatformActor::APlatformActor()
{
    BoxComponent = AddComponent<UBoxComponent>(FName("BoxComponent_0"));
    RootComponent = BoxComponent;

    MeshComponent = AddComponent<UStaticMeshComponent>(FName("MeshComponent_0"));
    MeshComponent->SetStaticMesh(UAssetManager::Get().GetStaticMesh(L"Contents/Oil/Oil.obj"));
    MeshComponent->SetupAttachment(BoxComponent);
}
