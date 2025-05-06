#pragma once

#include "Define.h"
#include "Hal/PlatformType.h"
#include "Container/Array.h"
#include "Math/Transform.h"

struct FBoneInfo
{
    FString Name;        // 뼈의 이름
    int32 Index = INDEX_NONE;      // SkeletonBones 배열 내에서 이 뼈의 인덱스
    int32 ParentIndex = INDEX_NONE; // SkeletonBones 배열 내에서 부모 뼈의 인덱스 (루트는 -1 또는 INDEX_NONE)
    
    // FbxNode* FbxNodePtr = nullptr;
};

struct FSkeletalMeshVertex
{
    float X = 0.f, Y = 0.f, Z = 0.f;
    float R = 0.5f, G = 0.5f, B = 0.5f, A = 0.5f;
    float NormalX = 0.f, NormalY = 0.f, NormalZ = 0.f;
    float TangentX = 0.f, TangentY = 0.f, TangentZ = 0.f, TangentW = 0.f;
    float U = 0, V = 0;
    uint32 BoneIndices[4] = { 0, 0, 0, 0 };
    float BoneWeights[4] = { 0.f, 0.f, 0.f, 0.f };

};

struct FSkeletalMeshRenderData
{
    FWString ObjectName;
    FString DisplayName;

    TArray<FSkeletalMeshVertex> Vertices;
    TArray<UINT> Indices;

    TArray<FObjMaterialInfo> Materials;
    TArray<FMaterialSubset> MaterialSubsets;

    FVector BoundingBoxMin;
    FVector BoundingBoxMax;

    TMap<class FbxNode*, int32> BoneNodeToIndexMap; // 파싱 이후에는 필요없음 빼야됨
    
    TArray<FBoneInfo> SkeletonBones;
    TArray<FMatrix> InverseGlobalBindPoseMatrices;
    // 각 뼈의 참조 포즈(바인드 포즈) 로컬 변환 저장
    TArray<FTransform> LocalBindPoseTransforms;
};
