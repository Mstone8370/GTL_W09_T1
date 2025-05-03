#include "SkeletalMesh.h"

#include "Asset/SkeletalMeshAsset.h"

#include "FbxLoader.h"

void USkeletalMesh::SetData(FSkeletalMeshRenderData* InRenderData)
{
    RenderData = InRenderData;


}

const FSkeletalMeshRenderData* USkeletalMesh::GetRenderData() const
{
    return RenderData.get(); 
}
