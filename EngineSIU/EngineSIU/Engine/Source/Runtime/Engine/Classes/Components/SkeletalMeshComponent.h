#pragma once
#include "SkinnedMeshComponent.h"

#include "Engine/SkeletalMesh.h"
#include "Engine/Asset/SkeletalMeshAsset.h"

class USkeletalMeshComponent : public USkinnedMeshComponent
{
    DECLARE_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)

public:
    USkeletalMeshComponent() = default;

    virtual ~USkeletalMeshComponent() override = default;
    virtual UObject* Duplicate(UObject* InOuter) override;

    void GetProperties(TMap<FString, FString>& OutProperties) const override;

    void SetProperties(const TMap<FString, FString>& InProperties) override;

    void SetselectedSubMeshIndex(const int& value) { selectedSubMeshIndex = value; }
    int GetselectedSubMeshIndex() const { return selectedSubMeshIndex; };

    virtual uint32 GetNumMaterials() const override;
    virtual UMaterial* GetMaterial(uint32 ElementIndex) const override;
    virtual uint32 GetMaterialIndex(FName MaterialSlotName) const override;
    virtual TArray<FName> GetMaterialSlotNames() const override;
    virtual void GetUsedMaterials(TArray<UMaterial*>& Out) const override;

    virtual int CheckRayIntersection(const FVector& InRayOrigin, const FVector& InRayDirection, float& OutHitDistance) const override;

    USkeletalMesh* GetSkeletalMesh() const { return SkeletalMesh; }
    void SetSkeletalMesh(USkeletalMesh* value)
    {
        SkeletalMesh = value;
        if (SkeletalMesh == nullptr)
        {
            OverrideMaterials.SetNum(0);
            AABB = FBoundingBox(FVector::ZeroVector, FVector::ZeroVector);
        }
        else
        {
            OverrideMaterials.SetNum(value->GetMaterials().Num());
            AABB = FBoundingBox(SkeletalMesh->GetRenderData()->BoundingBoxMin, SkeletalMesh->GetRenderData()->BoundingBoxMax);
        }
    }

protected:
    USkeletalMesh* SkeletalMesh = nullptr;
    int selectedSubMeshIndex = -1;
};
