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
        SkinnedVertices.Empty();
    }

    TArray<FSkeletalMeshVertex> SkinnedVertices;

    virtual bool GetCurrentLocalBoneTransforms(TArray<FTransform>& OutLocalTransforms) const;


private:
    USkeletalMesh* SkeletalMesh;
};
