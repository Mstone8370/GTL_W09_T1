#include "AssetManager.h"
#include "Engine.h"

#include <filesystem>
#include "Engine/FObjLoader.h"

TMap<FWString, UStaticMesh*> UAssetManager::StaticMeshAssetMap;
TMap<FWString, USkeletalMesh*> UAssetManager::SkeletalMeshAssetMap;
TMap<FString, FStaticMeshRenderData*> UAssetManager::StaticMeshRenderDataMap;
TMap<FString, FSkeletalMeshRenderData*> UAssetManager::SkeletalMeshRenderDataMap;
TMap<FString, UMaterial*> UAssetManager::MaterialAssetMap;

UAssetManager& UAssetManager::Get()
{
    //if (UAssetManager* Singleton = GEngine->AssetManager)
    //{
    //    return *Singleton;
    //}
    //else
    //{
    //    UE_LOG(ELogLevel::Error, "Cannot use AssetManager if no AssetManagerClassName is defined!");
    //    assert(0);
    //    return *new UAssetManager; // never calls this
    //}
    
    static UAssetManager Singleton;
    return Singleton;
}

void UAssetManager::InitAssetManager()
{
    AssetRegistry = std::make_unique<FAssetRegistry>();

    LoadObjFiles();
}

const TMap<FName, FAssetInfo>& UAssetManager::GetAssetRegistry()
{
    return AssetRegistry->PathNameToAssetInfo;
}

void UAssetManager::LoadObjFiles()
{
    const std::string BasePathName = "Contents/";

    // Obj 파일 로드
    
    for (const auto& Entry : std::filesystem::recursive_directory_iterator(BasePathName))
    {
        if (Entry.is_regular_file() && Entry.path().extension() == ".obj")
        {
            FAssetInfo NewAssetInfo;
            NewAssetInfo.AssetName = FName(Entry.path().filename().string());
            NewAssetInfo.PackagePath = FName(Entry.path().parent_path().string());
            NewAssetInfo.AssetType = EAssetType::StaticMesh; // obj 파일은 무조 건 StaticMesh
            NewAssetInfo.Size = static_cast<uint32>(std::filesystem::file_size(Entry.path()));
            
            AssetRegistry->PathNameToAssetInfo.Add(NewAssetInfo.AssetName, NewAssetInfo);
            
            FString MeshName = NewAssetInfo.PackagePath.ToString() + "/" + NewAssetInfo.AssetName.ToString();
            FObjManager::CreateStaticMesh(MeshName);
            // ObjFileNames.push_back(UGTLStringLibrary::StringToWString(Entry.path().string()));
            // FObjManager::LoadObjStaticMeshAsset(UGTLStringLibrary::StringToWString(Entry.path().string()));
        }
    }
}

USkeletalMesh* UAssetManager::GetSkeletalMesh(FWString name)
{
    return SkeletalMeshAssetMap[name];
}

UStaticMesh* UAssetManager::GetStaticMesh(FWString name)
{
    return StaticMeshAssetMap[name];
}
