#pragma once

#include <fbxsdk.h>

#include "Container/Map.h"
#include "HAL/PlatformType.h"

class USkeletalMesh;
struct FSkeletalMeshRenderData;

class FFbxLoader
{
public:
    FFbxLoader();
    ~FFbxLoader();

    bool LoadFBX(const FString& InFilePath, FSkeletalMeshRenderData& OutRenderData);

private:
    FbxManager* Manager;
    FbxImporter* Importer;
    FbxScene* Scene;

    void TraverseNodeRecursive(FbxNode* Node, FSkeletalMeshRenderData& OutRenderData);
    void ProcessMesh(FbxNode* Node, FSkeletalMeshRenderData& OutRenderData);
    bool FBXConvertScene();
};

class FFbxManager
{
public:
    static USkeletalMesh* GetSkeletalMesh(const FWString& FilePath);

protected:
    static std::unique_ptr<FSkeletalMeshRenderData> LoadFbxSkeletalMeshAsset(const FWString& FilePath);
    static USkeletalMesh* CreateMesh(const FWString& FilePath);

    inline static TMap<FString, USkeletalMesh*> SkeletalMeshMap;
};
