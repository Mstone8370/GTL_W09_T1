#pragma once

#include "Define.h"
#include "Hal/PlatformType.h"
#include "Container/Array.h"

struct FSkeletalMeshVertex
{
    float X, Y, Z;    // Position
    float R, G, B, A; // Color
    float NormalX, NormalY, NormalZ;
    float TangentX, TangentY, TangentZ, TangentW;
    float U = 0, V = 0;
    uint32 BoneIndices[4];
    float BoneWeights[4];
};

struct Bone
{
    FString Name;
    int ParentIndex;
    TArray<int> ChildrenIndex;
    FMatrix skinningMatrix;
    FMatrix GlobalTransform;
    FMatrix LocalTransform;
    FMatrix BindPose;
    FMatrix InverseBindPose;
};

struct FSkeletalMeshRenderData
{
    FWString ObjectName;
    FString DisplayName;

    TArray<FSkeletalMeshVertex> Vertices;
    TArray<UINT> Indices;

    TArray<Bone> Bones;
    TArray<FMaterialInfo> Materials;
    TArray<FMaterialSubset> MaterialSubsets;

    FVector BoundingBoxMin;
    FVector BoundingBoxMax;
};
