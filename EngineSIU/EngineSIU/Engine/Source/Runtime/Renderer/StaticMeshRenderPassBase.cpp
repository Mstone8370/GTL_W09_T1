
#include "StaticMeshRenderPassBase.h"

#include "Engine/Engine.h"
#include "UObject/UObjectIterator.h"
#include "Components/StaticMeshComponent.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "Engine/EditorEngine.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/Casts.h"
#include "Editor/PropertyEditor/ShowFlags.h"

FStaticMeshRenderPassBase::FStaticMeshRenderPassBase()
    : BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
{
}

FStaticMeshRenderPassBase::~FStaticMeshRenderPassBase()
{
    ReleaseShader();
}

void FStaticMeshRenderPassBase::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;

    CreateResource();
}

void FStaticMeshRenderPassBase::PrepareRenderArr()
{
    for (const auto iter : TObjectRange<UStaticMeshComponent>())
    {
        if (iter->IsA<UGizmoBaseComponent>() || iter->GetWorld() != GEngine->ActiveWorld)
        {
            continue;
        }
        StaticMeshComponents.Add(iter);
    }
}

void FStaticMeshRenderPassBase::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    PrepareRenderPass(Viewport);

    Render_Internal(Viewport);

    CleanUpRenderPass(Viewport);
}

void FStaticMeshRenderPassBase::ClearRenderArr()
{
    StaticMeshComponents.Empty();
}

void FStaticMeshRenderPassBase::Render_Internal(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    RenderAllStaticMeshes(Viewport);
}

void FStaticMeshRenderPassBase::RenderAllStaticMeshes(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    for (UStaticMeshComponent* Comp : StaticMeshComponents)
    {
        if (!Comp || !Comp->GetStaticMesh())
        {
            continue;
        }

        FStaticMeshRenderData* RenderData = Comp->GetStaticMesh()->GetRenderData();
        if (RenderData == nullptr)
        {
            continue;
        }

        UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);

        FMatrix WorldMatrix = Comp->GetWorldMatrix();
        FVector4 UUIDColor = Comp->EncodeUUID() / 255.0f;
        const bool bIsSelected = (Engine && Engine->GetSelectedActor() == Comp->GetOwner());

        UpdateObjectConstant(WorldMatrix, UUIDColor, bIsSelected);

        RenderPrimitive(RenderData, Comp->GetStaticMesh()->GetMaterials(), Comp->GetOverrideMaterials(), Comp->GetselectedSubMeshIndex());

        if (Viewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_AABB))
        {
            FEngineLoop::PrimitiveDrawBatch.AddAABBToBatch(Comp->GetBoundingBox(), Comp->GetWorldLocation(), WorldMatrix);
        }
    }
}

void FStaticMeshRenderPassBase::RenderPrimitive(FStaticMeshRenderData* RenderData, TArray<FMeshMaterial*> Materials, TArray<UMaterial*> OverrideMaterials, int32 SelectedSubMeshIndex) const
{
    UINT Stride = sizeof(FStaticMeshVertex);
    UINT Offset = 0;

    FVertexInfo VertexInfo;
    BufferManager->CreateVertexBuffer(RenderData->ObjectName, RenderData->Vertices, VertexInfo);

    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &Stride, &Offset);

    FIndexInfo IndexInfo;
    BufferManager->CreateIndexBuffer(RenderData->ObjectName, RenderData->Indices, IndexInfo);
    if (IndexInfo.IndexBuffer)
    {
        Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    }

    if (RenderData->MaterialSubsets.Num() == 0)
    {
        Graphics->DeviceContext->DrawIndexed(RenderData->Indices.Num(), 0, 0);
        return;
    }

    for (int SubMeshIndex = 0; SubMeshIndex < RenderData->MaterialSubsets.Num(); SubMeshIndex++)
    {
        uint32 MaterialIndex = RenderData->MaterialSubsets[SubMeshIndex].MaterialIndex;

        FSubMeshConstants SubMeshData = (SubMeshIndex == SelectedSubMeshIndex) ? FSubMeshConstants(true) : FSubMeshConstants(false);

        BufferManager->UpdateConstantBuffer(TEXT("FSubMeshConstants"), SubMeshData);

        if (OverrideMaterials[MaterialIndex] != nullptr)
        {
            MaterialUtils::UpdateMaterial(BufferManager, Graphics, OverrideMaterials[MaterialIndex]->GetMaterialInfo());
        }
        else
        {
            MaterialUtils::UpdateMaterial(BufferManager, Graphics, Materials[MaterialIndex]->Material->GetMaterialInfo());
        }

        uint32 StartIndex = RenderData->MaterialSubsets[SubMeshIndex].IndexStart;
        uint32 IndexCount = RenderData->MaterialSubsets[SubMeshIndex].IndexCount;
        Graphics->DeviceContext->DrawIndexed(IndexCount, StartIndex, 0);
    }
}

void FStaticMeshRenderPassBase::RenderPrimitive(ID3D11Buffer* Buffer, UINT VerticesNum) const
{
    UINT Stride = sizeof(FStaticMeshVertex);
    UINT Offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &Buffer, &Stride, &Offset);
    Graphics->DeviceContext->Draw(VerticesNum, 0);
}

void FStaticMeshRenderPassBase::RenderPrimitive(ID3D11Buffer* VertexBuffer, ID3D11Buffer* IndexBuffer, UINT IndicesNum) const
{
    UINT Stride = sizeof(FStaticMeshVertex);
    UINT Offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
    Graphics->DeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    Graphics->DeviceContext->DrawIndexed(IndicesNum, 0, 0);
}

void FStaticMeshRenderPassBase::UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const
{
    FObjectConstantBuffer ObjectData = {};
    ObjectData.WorldMatrix = WorldMatrix;
    ObjectData.InverseTransposedWorld = FMatrix::Transpose(FMatrix::Inverse(WorldMatrix));
    ObjectData.UUIDColor = UUIDColor;
    ObjectData.bIsSelected = bIsSelected;

    BufferManager->UpdateConstantBuffer(TEXT("FObjectConstantBuffer"), ObjectData);
}
