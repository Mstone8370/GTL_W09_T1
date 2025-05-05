#pragma once

#include <fbxsdk.h>

#include "Container/Map.h"
#include "HAL/PlatformType.h"
#include "Runtime/Core/Math/Vector.h"
#include "Runtime/Core/Math/Quat.h"

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

    void TraverseSkeletonNodeRecursive(FbxNode* Node, FSkeletalMeshRenderData& OutRenderData);
    void TraverseMeshNodeRecursive(FbxNode* Node, FSkeletalMeshRenderData& OutRenderData);
    void ProcessMesh(FbxNode* Node, FSkeletalMeshRenderData& OutRenderData);
    void ProcessSkeleton(FbxNode* Node, FSkeletalMeshRenderData& OutRenderData);

    FVector TransformToTranslation(FbxAMatrix BindMatrix);
    FQuat TransformToRotation(FbxAMatrix BindMatrix);
    FVector TransformToScale(FbxAMatrix BindMatrix);
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
