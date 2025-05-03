#pragma once

#include "Define.h"
#include "Hal/PlatformType.h"
#include "Container/Array.h"

struct FStaticMeshVertex
{
    float X, Y, Z;    // Position
    float R, G, B, A; // Color
    float NormalX, NormalY, NormalZ;
    float TangentX, TangentY, TangentZ, TangentW;
    float U = 0, V = 0;
    uint32 MaterialIndex;
};

struct FStaticMeshRenderData
{
    FWString ObjectName;
    FString DisplayName;

    TArray<FStaticMeshVertex> Vertices;
    TArray<UINT> Indices;

    TArray<FMaterialInfo> Materials;
    TArray<FMaterialSubset> MaterialSubsets;

    FVector BoundingBoxMin;
    FVector BoundingBoxMax;
};
