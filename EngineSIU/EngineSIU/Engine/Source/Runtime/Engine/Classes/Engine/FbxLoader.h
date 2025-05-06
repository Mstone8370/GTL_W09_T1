#pragma once

#include <fbxsdk.h>

#include "Container/String.h"
#include "SkeletalMesh.h"
#include "Asset/SkeletalMeshAsset.h"

class UStaticMesh;
class UMaterial;

class FFbxLoader
{
public:
    FFbxLoader();
    ~FFbxLoader();

    bool LoadFBX(const FString& InFilePath);
    bool ParseFBX(const FString& FbxFilePath, FFbxInfo& OutFbxInfo);
    void ParseMesh(FbxNode* Node, FFbxInfo& OutFbxInfo);
    void ParseBone(FbxNode* Node, FFbxInfo& OutFbxInfo);
    void PostProcessBone(FFbxInfo& OutFbxInfo);
    void ParseBoneRecursive(FbxNode* Node, FFbxInfo& OutFbxInfo);
    void ParseMeshesRecursive(FbxNode* Node, FFbxInfo& OutFbxInfo);
    TArray<FbxPose*> GetBindPoses(FbxScene* Scene);
    void ParseMaterial(FbxNode* Node, FFbxInfo& OutFbxInfo);
    FMatrix ConvertFbxMatrixToFMatrix(const FbxMatrix& fbxMat);
    bool CreateTextureFromFile(const FWString& Filename, bool bIsSRGB = true);
    FbxScene* GetScene() const { return Scene; }
private:
    FbxManager* Manager;
    FbxImporter* Importer;
    FbxScene* Scene;
    TMap<FString, int32> BoneNameToIndex;
};

class FFbxManager
{
public:
    static FStaticMeshRenderData* LoadFbxStaticMeshAsset(const FString& PathFileName);

    static FSkeletalMeshRenderData* LoadFbxSkeletalMeshAsset(const FString& PathFileName);

    static void CreateMesh(const FString& filePath);

    static bool IsFbxSkeletalMesh(FbxScene* Scene);

    static UStaticMesh* CreateStaticMesh(const FString& filePath);

    static USkeletalMesh* CreateSkeletalMesh(const FString& filePath);

    static FVector SkinVertexPosition(const FSkeletalMeshVertex& vertex, const TArray<Bone>& bones);

    static void ConvertRawToSkeletalMeshRenderData(const FFbxInfo& Raw, FSkeletalMeshRenderData& Cooked);

    static void ConvertRawToStaticMeshRenderData(const FFbxInfo& Raw, FStaticMeshRenderData& Cooked);

    static UMaterial* CreateMaterial(FMaterialInfo materialInfo);

    /*static void RotateBone(FSkeletalMeshRenderData& SkeletalMesh, int32 BoneIndex, const FRotator& Rotation);
    static void TranslateBone(FSkeletalMeshRenderData& SkeletalMesh, int32 BoneIndex, const FVector& Translation);*/
    static void UpdateBoneGlobalTransformRecursive(TArray<Bone>& Bones, Bone& CurrentBone);
private:
    static FFbxLoader FbxLoader;
};
