#include "SkeletalMesh.h"

#include "Asset/SkeletalMeshAsset.h"

#include "FbxLoader.h"

void USkeletalMesh::SetData(FSkeletalMeshRenderData* InRenderData)
{
    RenderData = InRenderData;


}
