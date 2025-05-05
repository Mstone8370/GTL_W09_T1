#pragma once

#include "Define.h"
#include "Hal/PlatformType.h"
#include "Container/Array.h"
#include "Runtime/Core/Math/Quat.h"
struct FTransform
{
    FVector Translation = FVector::Zero();
    FQuat Rotation = FQuat::Identity;
    FVector Scale3D = FVector(1.0f, 1.0f, 1.0f);

    FMatrix ToMatrixWithScale() const
    {
        FMatrix RotMatrix = Rotation.ToMatrix();
        RotMatrix.M[0][0] *= Scale3D.X;
        RotMatrix.M[1][1] *= Scale3D.Y;
        RotMatrix.M[2][2] *= Scale3D.Z;

        RotMatrix.M[0][3] = Translation.X;
        RotMatrix.M[1][3] = Translation.Y;
        RotMatrix.M[2][3] = Translation.Z;

        return RotMatrix;
    }

    FTransform GetRelativeTransform(const FTransform& Parent) const
    {
        FQuat InvParentRot = Parent.Rotation.Inverse();
        FVector RelativeLoc = InvParentRot.RotateVector(Translation - Parent.Translation) / Parent.Scale3D;
        FQuat RelativeRot = InvParentRot * Rotation;
        FVector RelativeScale = Scale3D / Parent.Scale3D;
        return FTransform{ RelativeLoc, RelativeRot, RelativeScale };
    }

    static FTransform Identity()
    {
        return FTransform(FVector::Zero(), FQuat::Identity, FVector(1, 1, 1));
    }

    FTransform() = default;
    FTransform(const FVector& InTrans, const FQuat& InRot, const FVector& InScale)
        : Translation(InTrans), Rotation(InRot), Scale3D(InScale)
    {
    }
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

    TArray<FTransform> BoneBindPoseTransforms;  // 본의 바인드 포즈
    TArray<FTransform> BoneLocalTransforms;     // 본의 로컬 트랜스폼
    TArray<FName> BoneNames;                    // 본 이름
    TArray<int32> BoneParents;                  // 본의 부모 인덱스
};
