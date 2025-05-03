#include "SkeletalMeshRenderPassBase.h"

#include "UObject/UObjectIterator.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/EditorEngine.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/Casts.h"
#include "Editor/PropertyEditor/ShowFlags.h"
#include "Engine/Asset/SkeletalMeshAsset.h"

class UEditorEngine;

FSkeletalMeshRenderPassBase::FSkeletalMeshRenderPassBase()
    : BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
{
}

FSkeletalMeshRenderPassBase::~FSkeletalMeshRenderPassBase()
{
    ReleaseResource();
}

void FSkeletalMeshRenderPassBase::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;

    CreateResource();
}

void FSkeletalMeshRenderPassBase::PrepareRenderArr()
{
    for (const auto iter : TObjectRange<USkeletalMeshComponent>())
    {
        if (iter->GetWorld() != GEngine->ActiveWorld)
        {
            continue;
        }
        SkeletalMeshComponents.Add(iter);
    }
}

void FSkeletalMeshRenderPassBase::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    PrepareRenderPass(Viewport);

    Render_Internal(Viewport);

    CleanUpRenderPass(Viewport);
}

void FSkeletalMeshRenderPassBase::ClearRenderArr()
{
    SkeletalMeshComponents.Empty();
}

void FSkeletalMeshRenderPassBase::Render_Internal(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    RenderAllSkeletalMeshes(Viewport);
}

void FSkeletalMeshRenderPassBase::RenderAllSkeletalMeshes(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    for (USkeletalMeshComponent* Comp : SkeletalMeshComponents)
    {
        if (!Comp || !Comp->GetSkeletalMesh())
        {
            continue;
        }

        const FSkeletalMeshRenderData* RenderData = Comp->GetSkeletalMesh()->GetRenderData();
        if (RenderData == nullptr)
        {
            continue;
        }

        UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);

        FMatrix WorldMatrix = Comp->GetWorldMatrix();
        FVector4 UUIDColor = Comp->EncodeUUID() / 255.0f;
        const bool bIsSelected = (Engine && Engine->GetSelectedActor() == Comp->GetOwner());

        UpdateObjectConstant(WorldMatrix, UUIDColor, bIsSelected);

        RenderSkeletalMesh(RenderData);

        if (Viewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_AABB))
        {
            FEngineLoop::PrimitiveDrawBatch.AddAABBToBatch(Comp->GetBoundingBox(), Comp->GetWorldLocation(), WorldMatrix);
        }
    }
}

void FSkeletalMeshRenderPassBase::RenderSkeletalMesh(const FSkeletalMeshRenderData* RenderData) const
{
    UINT Stride = sizeof(FSkeletalMeshVertex);
    UINT Offset = 0;

    FVertexInfo VertexInfo;
    BufferManager->CreateVertexBuffer(RenderData->ObjectName, RenderData->Vertices, VertexInfo);

    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &Stride, &Offset);

    FIndexInfo IndexInfo;
    BufferManager->CreateIndexBuffer(RenderData->ObjectName, RenderData->Indices, IndexInfo);
    if (IndexInfo.IndexBuffer)
    {
        Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        Graphics->DeviceContext->DrawIndexed(RenderData->Indices.Num(), 0, 0);
    }
    else
    {
        Graphics->DeviceContext->Draw(RenderData->Vertices.Num(), 0);
    }
}

void FSkeletalMeshRenderPassBase::RenderSkeletalMesh(ID3D11Buffer* Buffer, UINT VerticesNum) const
{
}

void FSkeletalMeshRenderPassBase::RenderSkeletalMesh(ID3D11Buffer* VertexBuffer, ID3D11Buffer* IndexBuffer, UINT IndicesNum) const
{
}

void FSkeletalMeshRenderPassBase::UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const
{
    FObjectConstantBuffer ObjectData = {};
    ObjectData.WorldMatrix = WorldMatrix;
    ObjectData.InverseTransposedWorld = FMatrix::Transpose(FMatrix::Inverse(WorldMatrix));
    ObjectData.UUIDColor = UUIDColor;
    ObjectData.bIsSelected = bIsSelected;

    BufferManager->UpdateConstantBuffer(TEXT("FObjectConstantBuffer"), ObjectData);
}
