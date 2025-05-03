#pragma once

#include <fbxsdk.h>

#include "Container/String.h"
#include "SkeletalMesh.h"

struct FSkeletalMeshRenderData;
struct FStaticMeshRenderData;

class UStaticMesh;
class UMaterial;

class FFbxLoader
{
public:
    FFbxLoader();
    ~FFbxLoader();

    bool LoadFBX(const FString& InFilePath);
    bool ParseFBX(const FString& FbxFilePath, FFbxInfo& OutFbxInfo);
    void ParseFbxNodeRecursive(FbxNode* Node, FFbxInfo& OutFbxInfo);
    void ParseMesh(FbxNode* Node, FFbxInfo& OutFbxInfo);
    void ParseBone(FbxNode* Node, FFbxInfo& OutFbxInfo);
    void ParseMaterial(FbxNode* Node, FFbxInfo& OutFbxInfo);
    FMatrix ConvertFbxMatrixToFMatrix(const FbxAMatrix& fbxMat);
    FbxScene* GetScene() const { return Scene; }
private:
    FbxManager* Manager;
    FbxImporter* Importer;
    FbxScene* Scene;
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
    static void ConvertRawToSkeletalMeshRenderData(const FFbxInfo& Raw, FSkeletalMeshRenderData& Cooked);
    static void ConvertRawToStaticMeshRenderData(const FFbxInfo& Raw, FStaticMeshRenderData& Cooked);
private:
    static FFbxLoader FbxLoader;
};
