#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "Components/Material/Material.h"
#include "Define.h"

struct FStaticMeshRenderData;

class UStaticMesh : public UObject
{
    DECLARE_CLASS(UStaticMesh, UObject)

public:
    UStaticMesh() = default;

    virtual UObject* Duplicate(UObject* InOuter) override;

    const TArray<FMeshMaterial*>& GetMaterials() const { return materials; }
    uint32 GetMaterialIndex(FName MaterialSlotName) const;
    void GetUsedMaterials(TArray<UMaterial*>& OutMaterial) const;
    FStaticMeshRenderData* GetRenderData() const { return RenderData; }

    //ObjectName은 경로까지 포함
    FWString GetOjbectName() const;

    void SetData(FStaticMeshRenderData* InRenderData);

private:
    FStaticMeshRenderData* RenderData = nullptr;
    TArray<FMeshMaterial*> materials;
};
