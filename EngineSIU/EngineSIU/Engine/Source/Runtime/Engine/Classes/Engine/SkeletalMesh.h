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

    virtual UObject* Duplicate(UObject* InOuter) override;

    const TArray<FMeshMaterial*>& GetMaterials() const { return materials; }
    uint32 GetMaterialIndex(FName MaterialSlotName) const;
    void GetUsedMaterials(TArray<UMaterial*>& OutMaterial) const;
    FSkeletalMeshRenderData* GetRenderData() const;

    //ObjectName은 경로까지 포함
    FWString GetOjbectName() const;

    void SetData(FSkeletalMeshRenderData* InRenderData);
protected:
    std::unique_ptr<FSkeletalMeshRenderData> RenderData;
    TArray<FMeshMaterial*> materials;
};
