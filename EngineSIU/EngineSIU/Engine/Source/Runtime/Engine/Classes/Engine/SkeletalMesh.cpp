#include "SkeletalMesh.h"

#include "Asset/SkeletalMeshAsset.h"

#include "FbxLoader.h"

UObject* USkeletalMesh::Duplicate(UObject* InOuter)
{
    return nullptr;
}

uint32 USkeletalMesh::GetMaterialIndex(FName MaterialSlotName) const
{
    for (uint32 materialIndex = 0; materialIndex < materials.Num(); materialIndex++) {
        if (materials[materialIndex]->MaterialSlotName == MaterialSlotName)
            return materialIndex;
    }

    return -1;
}

void USkeletalMesh::GetUsedMaterials(TArray<UMaterial*>& OutMaterial) const
{
    for (const FMeshMaterial* Material : materials)
    {
        OutMaterial.Emplace(Material->Material);
    }
}

FSkeletalMeshRenderData* USkeletalMesh::GetRenderData() const
{
    return RenderData.get();
}

FWString USkeletalMesh::GetOjbectName() const
{
    return RenderData->ObjectName;
}

void USkeletalMesh::SetData(FSkeletalMeshRenderData* InRenderData)
{
    RenderData.reset(InRenderData);

    for (int materialIndex = 0; materialIndex < RenderData->Materials.Num(); materialIndex++)
    {
        FMeshMaterial* newMaterialSlot = new FMeshMaterial();
        UMaterial* newMaterial = FFbxManager::CreateMaterial(RenderData->Materials[materialIndex]);

        newMaterialSlot->Material = newMaterial;
        newMaterialSlot->MaterialSlotName = RenderData->Materials[materialIndex].MaterialName;

        materials.Add(newMaterialSlot);
    }
}
