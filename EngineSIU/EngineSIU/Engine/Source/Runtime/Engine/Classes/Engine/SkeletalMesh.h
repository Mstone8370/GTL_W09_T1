#pragma once
#include "SkinnedAsset.h"
#include "Asset/StaticMeshAsset.h"

struct FSkeletalMeshRenderData;

class USkeletalMesh : public USkinnedAsset
{
    DECLARE_CLASS(USkeletalMesh, USkinnedAsset)

public:
    USkeletalMesh() = default;
    virtual ~USkeletalMesh() override = default;

    void SetData(FSkeletalMeshRenderData* InRenderData);

    const FSkeletalMeshRenderData* GetRenderData() const;

protected:
    std::unique_ptr<FSkeletalMeshRenderData> RenderData;
};
