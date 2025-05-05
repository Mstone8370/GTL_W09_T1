#pragma once
#include "SkinnedMeshComponent.h"
#include "Engine/Asset/SkeletalMeshAsset.h"

class USkeletalMesh;

class USkeletalMeshComponent : public USkinnedMeshComponent
{
    DECLARE_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)

public:
    USkeletalMeshComponent() = default;
    virtual ~USkeletalMeshComponent() override = default;

    USkeletalMesh* GetSkeletalMesh() const { return SkeletalMesh; }
    void SetSkeletalMesh(USkeletalMesh* InSkeletalMesh)
    {
        SkeletalMesh = InSkeletalMesh;
        SkinnedPositions.Empty();
        SkinnedVertices.Empty(); 
        SkinnedNormals.Empty();
    }
    
    // --- CPU 스키닝 결과 저장용 ---
    // 구조체로 리펙토링 
    // 실제로는 위치, 노멀, 탄젠트를 포함하는 구조체 배열이 더 효율적일 수 있음
    TArray<FVector> SkinnedPositions;
    TArray<FVector> SkinnedNormals;
    TArray<FSkeletalMeshVertex> SkinnedVertices;

    virtual bool GetCurrentLocalBoneTransforms(TArray<FTransform>& OutLocalTransforms) const;


private:
    USkeletalMesh* SkeletalMesh;
};
