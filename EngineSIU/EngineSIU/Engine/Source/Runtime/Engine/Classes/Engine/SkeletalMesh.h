#pragma once
#include "SkinnedAsset.h"

struct FSkeletalMeshRenderData;

class USkeletalMesh : public USkinnedAsset
{
    DECLARE_CLASS(USkeletalMesh, USkinnedAsset)

public:
    USkeletalMesh() = default;
    virtual ~USkeletalMesh() override = default;

    void SetData(FSkeletalMeshRenderData* InRenderData);

private:
    FSkeletalMeshRenderData* RenderData = nullptr;
};
