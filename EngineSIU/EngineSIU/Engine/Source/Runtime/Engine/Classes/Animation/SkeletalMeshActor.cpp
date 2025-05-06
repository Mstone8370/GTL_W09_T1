#include "SkeletalMeshActor.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/AssetManager.h"

ASkeletalMeshActor::ASkeletalMeshActor()
{
    SkeletalMeshComponent = AddComponent<USkeletalMeshComponent>("SkeletalMeshComponent_0");
    RootComponent = SkeletalMeshComponent;
    //SkeletalMeshComponent->SetSkeletalMesh(UAssetManager::Get().GetSkeletalMeshAsset(L"X Bot Triangle.fbx"));
    //SkeletalMeshComponent->SetSkeletalMesh(UAssetManager::Get().GetSkeletalMeshAsset(L"X Bot.fbx"));
    //SkeletalMeshComponent->SetSkeletalMesh(UAssetManager::Get().GetSkeletalMeshAsset(L"c1280/c1280.fbx"));
    SkeletalMeshComponent->SetSkeletalMesh(UAssetManager::Get().GetSkeletalMeshAsset(L"Mutant.fbx"));
    //SkeletalMeshComponent->SetSkeletalMesh(UAssetManager::Get().GetSkeletalMeshAsset(L"Ch20_nonPBR.fbx"));
}
