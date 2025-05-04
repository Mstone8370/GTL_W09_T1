#include "SkeletalMeshActor.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/AssetManager.h"

ASkeletalMeshActor::ASkeletalMeshActor()
{
    SkeletalMeshComponent = AddComponent<USkeletalMeshComponent>("SkeletalMeshComponent_0");
    RootComponent = SkeletalMeshComponent;
    SkeletalMeshComponent->SetSkeletalMesh(UAssetManager::Get().GetSkeletalMeshAsset(L"XBotTriangle.fbx"));
}
